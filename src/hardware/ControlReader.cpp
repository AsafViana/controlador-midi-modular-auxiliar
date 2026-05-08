#include "ControlReader.h"
#include "../config.h"
#include "HardwareMap.h"
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

// --- Internal state ---

struct ButtonState {
  bool lastReading;
  bool stableState;
  uint32_t lastChangeMs;
};

struct EncoderState {
  uint8_t lastAB;
  uint8_t value;
};

struct AnalogState {
  uint8_t lastValue;
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
    encoderStates[i] = {0, MIDI_MID};
    analogStates[i] = {0};
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
    bool invertido = HardwareMap::isInvertido(i);

    switch (tipo) {
    case TipoControle::POTENCIOMETRO:
    case TipoControle::SENSOR: {
      uint16_t adcRaw = analogRead(gpio);
      uint8_t midiValue = mapAdcToMidi(adcRaw);

      if (applyDeadzone(midiValue, analogStates[i].lastValue, DEADZONE)) {
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
                        buttonStates[i].lastChangeMs, nowMs, DEBOUNCE_MS);

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

      uint8_t newValue = processEncoderTransition(
          encoderStates[i].lastAB, currentAB, encoderStates[i].value);

      encoderStates[i].lastAB = currentAB;
      encoderStates[i].value = newValue;
      valueBuffer[i] = invertido ? invertValue(newValue) : newValue;
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

uint8_t getNumControles() { return HardwareMap::NUM_CONTROLES; }

} // namespace ControlReader
