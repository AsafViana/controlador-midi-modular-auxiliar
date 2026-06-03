#include "ControlReader.h"
#include "../config.h"
#include "Calibration.h"
#include "HardwareMap.h"
#include "PersistentConfig.h"
#include <Arduino.h>

namespace ControlReader {

// --- Internal state ---

struct ButtonState {
  bool lastReading;
  bool stableState;
  uint32_t lastChangeMs;
};

struct EncoderState {
  uint8_t lastAB;
  uint8_t value;
  uint32_t lastTransitionMs;
};

struct AnalogState {
  uint8_t lastValue;
  uint16_t filteredAdc; // Valor ADC filtrado por EMA (mantém resolução 12-bit)
  uint8_t minInWindow;  // Mínimo MIDI na janela de detecção
  uint8_t maxInWindow;  // Máximo MIDI na janela de detecção
  uint32_t windowStartMs; // Início da janela de detecção
  bool disconnected;      // Flag de pino flutuante detectado
  uint32_t lastReadMs;    // Rate limiting: último momento de leitura
};

static volatile uint8_t valueBuffer[MAX_CONTROLES];
static ButtonState buttonStates[MAX_CONTROLES];
static EncoderState encoderStates[MAX_CONTROLES];
static AnalogState analogStates[MAX_CONTROLES];

// Índices pré-calculados dos push-buttons de encoder no buffer
static uint8_t encoderSwitchSlots[MAX_CONTROLES];

static volatile bool hasChange = false;

void init() {
  // Initialize buffer to zeros
  for (uint8_t i = 0; i < MAX_CONTROLES; i++) {
    valueBuffer[i] = 0;
    buttonStates[i] = {false, false, 0};
    encoderStates[i] = {0, MIDI_MID, 0};
    analogStates[i] = {0, 0, 127, 0, 0, false, 0};
    encoderSwitchSlots[i] = 0;
  }

  // Pré-calcula os slots dos push-buttons de encoder (evita busca O(n) no loop)
  uint8_t swSlot = HardwareMap::NUM_CONTROLES;
  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    if (HardwareMap::getTipo(i) == TipoControle::ENCODER &&
        HardwareMap::getGpioSwitch(i) > 0) {
      encoderSwitchSlots[i] = swSlot;
      swSlot++;
    }
  }

  // Configure GPIOs based on HardwareMap
  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    TipoControle tipo = HardwareMap::getTipo(i);
    uint8_t gpio = HardwareMap::getGpio(i);

    switch (tipo) {
    case TipoControle::BOTAO:
      pinMode(gpio, INPUT);
      break;
    case TipoControle::POTENCIOMETRO:
    case TipoControle::SENSOR:
      pinMode(gpio, INPUT);
      break;
    case TipoControle::ENCODER:
      pinMode(gpio, INPUT);
      pinMode(HardwareMap::getGpioB(i), INPUT);
      // Configura push-button do encoder se definido
      if (HardwareMap::getGpioSwitch(i) > 0) {
        pinMode(HardwareMap::getGpioSwitch(i), INPUT);
      }
      // Initialize encoder value at MIDI_MID
      valueBuffer[i] = MIDI_MID;
      break;
    }
  }

  // Configura pino de interrupção como open-drain para wire-OR
  // Idle = HIGH-Z (pull-up externo no Master mantém HIGH)
  // Ativo = LOW (sinaliza dados novos)
  pinMode(PIN_INT_OUT, OUTPUT_OPEN_DRAIN);
  digitalWrite(PIN_INT_OUT, HIGH); // HIGH = high-impedance (release)
  hasChange = false;
}

void update() {
  uint32_t nowMs = millis();

  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    TipoControle tipo = HardwareMap::getTipo(i);
    uint8_t gpio = HardwareMap::getGpio(i);
    auto cfg = PersistentConfig::getConfig(i);
    bool invertido = cfg.invertido;

    switch (tipo) {
    case TipoControle::POTENCIOMETRO:
    case TipoControle::SENSOR: {
      // Rate limiting: ler analógicos a cada ANALOG_READ_INTERVAL_MS
      if ((nowMs - analogStates[i].lastReadMs) < ANALOG_READ_INTERVAL_MS)
        break;
      analogStates[i].lastReadMs = nowMs;

      uint16_t adcRaw = analogRead(gpio);

      // Alimenta calibração se em modo calibração
      Calibration::feedRawValue(i, adcRaw);

      // Aplica calibração: mapeia [min,max] → [0, 4095]
      auto cal = Calibration::getChannelCal(i);
      uint16_t adcCal = Calibration::applyCal(adcRaw, cal.adcMin, cal.adcMax);

      // Aplica suavização EMA no valor ADC (mantém resolução 12-bit)
      analogStates[i].filteredAdc =
          applyEma(analogStates[i].filteredAdc, adcCal, EMA_ALPHA);

      uint8_t midiValue = mapAdcToMidi(analogStates[i].filteredAdc);

      // Detecção de pino flutuante: variância alta em janela curta
      if ((nowMs - analogStates[i].windowStartMs) >= FLOAT_DETECT_WINDOW_MS) {
        // Fim da janela: verifica variância
        uint8_t range =
            analogStates[i].maxInWindow - analogStates[i].minInWindow;
        analogStates[i].disconnected = (range >= FLOAT_DETECT_THRESHOLD);
        // Reset janela
        analogStates[i].minInWindow = midiValue;
        analogStates[i].maxInWindow = midiValue;
        analogStates[i].windowStartMs = nowMs;
      } else {
        // Atualiza min/max na janela
        if (midiValue < analogStates[i].minInWindow)
          analogStates[i].minInWindow = midiValue;
        if (midiValue > analogStates[i].maxInWindow)
          analogStates[i].maxInWindow = midiValue;
      }

      // Se desconectado, congela último valor válido
      if (analogStates[i].disconnected)
        break;

      if (applyDeadzone(midiValue, analogStates[i].lastValue, cfg.deadzone)) {
        analogStates[i].lastValue = midiValue;
        valueBuffer[i] = invertido ? invertValue(midiValue) : midiValue;
        hasChange = true;
      }
      break;
    }

    case TipoControle::BOTAO: {
      bool reading =
          (digitalRead(gpio) == LOW); // Pull-up externo: LOW = pressed

      // Track when reading changes from last reading
      if (reading != buttonStates[i].lastReading) {
        buttonStates[i].lastChangeMs = nowMs;
        buttonStates[i].lastReading = reading;
      }

      bool newStable =
          applyDebounce(reading, buttonStates[i].stableState,
                        buttonStates[i].lastChangeMs, nowMs, cfg.debounce);

      if (newStable != buttonStates[i].stableState) {
        buttonStates[i].stableState = newStable;
        // pressed = 127, not pressed = 0; invert if invertido
        uint8_t val = newStable ? MIDI_MAX : 0;
        valueBuffer[i] = invertido ? invertValue(val) : val;
        hasChange = true;
      }
      break;
    }

    case TipoControle::ENCODER: {
      uint8_t a = digitalRead(gpio) ? 1 : 0;
      uint8_t b = digitalRead(HardwareMap::getGpioB(i)) ? 1 : 0;
      uint8_t currentAB = (a << 1) | b;

      if (currentAB != encoderStates[i].lastAB) {
        uint32_t elapsed = nowMs - encoderStates[i].lastTransitionMs;
        uint8_t newValue =
            processEncoderWithAccel(encoderStates[i].lastAB, currentAB,
                                    encoderStates[i].value, elapsed);

        encoderStates[i].lastAB = currentAB;
        encoderStates[i].value = newValue;
        encoderStates[i].lastTransitionMs = nowMs;
        valueBuffer[i] = invertido ? invertValue(newValue) : newValue;
        hasChange = true;
      }

      // Push-button integrado do encoder (usa slot pré-calculado)
      uint8_t gpioSw = HardwareMap::getGpioSwitch(i);
      if (gpioSw > 0) {
        uint8_t swIdx = encoderSwitchSlots[i];
        if (swIdx < MAX_CONTROLES) {
          bool reading =
              (digitalRead(gpioSw) == LOW); // Pull-up externo: LOW = pressed

          if (reading != buttonStates[swIdx].lastReading) {
            buttonStates[swIdx].lastChangeMs = nowMs;
            buttonStates[swIdx].lastReading = reading;
          }

          bool newStable = applyDebounce(
              reading, buttonStates[swIdx].stableState,
              buttonStates[swIdx].lastChangeMs, nowMs, DEBOUNCE_MS);

          if (newStable != buttonStates[swIdx].stableState) {
            buttonStates[swIdx].stableState = newStable;
            uint8_t val = newStable ? MIDI_MAX : 0;
            valueBuffer[swIdx] = invertido ? invertValue(val) : val;
            hasChange = true;
          }
        }
      }
      break;
    }
    }
  }

  // Se houve qualquer mudança, puxa INT_OUT para LOW para sinalizar o Master
  if (hasChange) {
    digitalWrite(PIN_INT_OUT, LOW);
  }
}

const volatile uint8_t *getValues() { return valueBuffer; }

uint8_t getValue(uint8_t index) {
  if (index < HardwareMap::TOTAL_SLOTS) {
    return valueBuffer[index];
  }
  return 0;
}

uint8_t getNumControles() { return HardwareMap::TOTAL_SLOTS; }

void signalChange() {
  hasChange = true;
  digitalWrite(PIN_INT_OUT, LOW); // Assert: puxa linha para LOW
}

void clearSignal() {
  hasChange = false;
  digitalWrite(PIN_INT_OUT, HIGH); // Release: high-impedance (open-drain)
}

} // namespace ControlReader
