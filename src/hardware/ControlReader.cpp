#include "ControlReader.h"
#include "../config.h"
#include "Calibration.h"
#include "HardwareMap.h"
#include "PersistentConfig.h"
#include <Arduino.h>

namespace ControlReader {

// --- Pure processing functions ---

uint8_t mapAdcToMidi(uint16_t adcValue) {
  return static_cast<uint8_t>(static_cast<uint32_t>(adcValue) * MIDI_MAX /
                              ADC_MAX);
}

bool applyDeadzone(uint8_t newValue, uint8_t lastValue, uint8_t deadzone) {
  uint8_t diff =
      (newValue > lastValue) ? (newValue - lastValue) : (lastValue - newValue);
  return diff > deadzone;
}

bool applyDebounce(bool reading, bool lastStable, uint32_t lastChangeMs,
                   uint32_t nowMs, uint16_t debounceMs) {
  if (reading != lastStable) {
    // Input differs from stable state — only transition if stable long
    // enough
    if ((nowMs - lastChangeMs) >= debounceMs) {
      return reading;
    }
  }
  return lastStable;
}

uint8_t invertValue(uint8_t value) { return MIDI_MAX - value; }

uint8_t processEncoderTransition(uint8_t lastAB, uint8_t currentAB,
                                 uint8_t currentValue) {
  // Quadrature decoding lookup table
  // Index = (lastAB << 2) | currentAB
  // Values: 0 = no change, 1 = CW, -1 = CCW
  static const int8_t quadratureTable[16] = {
      //  cur: 00  01  10  11
      /*prev 00*/ 0,  1,  -1, 0,
      /*prev 01*/ -1, 0,  0,  1,
      /*prev 10*/ 1,  0,  0,  -1,
      /*prev 11*/ 0,  -1, 1,  0};

  uint8_t index = ((lastAB & 0x03) << 2) | (currentAB & 0x03);
  int8_t direction = quadratureTable[index];

  if (direction == 1) {
    // CW: increment, bounded at 127
    return (currentValue < MIDI_MAX) ? (currentValue + 1) : MIDI_MAX;
  } else if (direction == -1) {
    // CCW: decrement, bounded at 0
    return (currentValue > 0) ? (currentValue - 1) : 0;
  }

  // Invalid transition or no change
  return currentValue;
}

uint16_t applyEma(uint16_t currentFiltered, uint16_t newRaw, uint8_t alpha) {
  // EMA: filtered = alpha * new + (1 - alpha) * filtered
  // Using fixed-point: result = (alpha * new + (256 - alpha) * filtered) / 256
  uint32_t result = (uint32_t)alpha * newRaw +
                    (uint32_t)(EMA_SCALE - alpha) * currentFiltered;
  return (uint16_t)(result / EMA_SCALE);
}

uint8_t processEncoderWithAccel(uint8_t lastAB, uint8_t currentAB,
                                uint8_t currentValue, uint32_t elapsedMs) {
  // Determina direção usando a tabela de quadratura
  static const int8_t quadratureTable[16] = {0, 1, -1, 0,  -1, 0,  0, 1,
                                             1, 0, 0,  -1, 0,  -1, 1, 0};

  uint8_t index = ((lastAB & 0x03) << 2) | (currentAB & 0x03);
  int8_t direction = quadratureTable[index];

  if (direction == 0)
    return currentValue;

  // Calcula step baseado na velocidade
  uint8_t step = 1;
  if (elapsedMs <= ENC_ACCEL_FAST_MS) {
    step = ENC_ACCEL_FAST_STEP;
  } else if (elapsedMs <= ENC_ACCEL_MED_MS) {
    step = ENC_ACCEL_MED_STEP;
  }

  if (direction == 1) {
    uint8_t newVal = currentValue + step;
    return (newVal > MIDI_MAX || newVal < currentValue) ? MIDI_MAX : newVal;
  } else {
    return (currentValue >= step) ? (currentValue - step) : 0;
  }
}

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
};

static volatile uint8_t valueBuffer[MAX_CONTROLES];
static ButtonState buttonStates[MAX_CONTROLES];
static EncoderState encoderStates[MAX_CONTROLES];
static AnalogState analogStates[MAX_CONTROLES];

// --- Hardware-dependent functions ---

void init() {
  // Initialize buffer to zeros
  for (uint8_t i = 0; i < MAX_CONTROLES; i++) {
    valueBuffer[i] = 0;
    buttonStates[i] = {false, false, 0};
    encoderStates[i] = {0, MIDI_MID, 0};
    analogStates[i] = {0, 0, 127, 0, 0, false};
  }

  // Configure GPIOs based on HardwareMap
  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    TipoControle tipo = HardwareMap::getTipo(i);
    uint8_t gpio = HardwareMap::getGpio(i);

    switch (tipo) {
    case TipoControle::BOTAO:
      pinMode(gpio, INPUT_PULLUP);
      break;
    case TipoControle::POTENCIOMETRO:
    case TipoControle::SENSOR:
      pinMode(gpio, INPUT);
      break;
    case TipoControle::ENCODER:
      pinMode(gpio, INPUT_PULLUP);
      pinMode(HardwareMap::getGpioB(i), INPUT_PULLUP);
      // Configura push-button do encoder se definido
      if (HardwareMap::getGpioSwitch(i) > 0) {
        pinMode(HardwareMap::getGpioSwitch(i), INPUT_PULLUP);
      }
      // Initialize encoder value at MIDI_MID
      valueBuffer[i] = MIDI_MID;
      break;
    }
  }
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
      }
      break;
    }

    case TipoControle::BOTAO: {
      bool reading = (digitalRead(gpio) == LOW); // INPUT_PULLUP: LOW = pressed

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
      }

      // Push-button integrado do encoder (ocupa slot extra no buffer)
      uint8_t gpioSw = HardwareMap::getGpioSwitch(i);
      if (gpioSw > 0) {
        // Calcula o índice do slot do push-button: NUM_CONTROLES + offset
        uint8_t swIdx = HardwareMap::NUM_CONTROLES;
        for (uint8_t j = 0; j < i; j++) {
          if (HardwareMap::getTipo(j) == TipoControle::ENCODER &&
              HardwareMap::getGpioSwitch(j) > 0) {
            swIdx++;
          }
        }
        if (swIdx < MAX_CONTROLES) {
          bool reading = (digitalRead(gpioSw) == LOW);

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
          }
        }
      }
      break;
    }
    }
  }
}

const volatile uint8_t *getValues() { return valueBuffer; }

uint8_t getValue(uint8_t index) {
  if (index < HardwareMap::NUM_CONTROLES) {
    return valueBuffer[index];
  }
  return 0;
}

uint8_t getNumControles() { return HardwareMap::TOTAL_SLOTS; }

} // namespace ControlReader
