// Pure function implementations for native testing (no Arduino.h dependency)
#include "../../src/config.h"
#include "../../src/hardware/ControlReader.h"

namespace ControlReader {

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
    if ((nowMs - lastChangeMs) >= debounceMs) {
      return reading;
    }
  }
  return lastStable;
}

uint8_t invertValue(uint8_t value) { return MIDI_MAX - value; }

uint8_t processEncoderTransition(uint8_t lastAB, uint8_t currentAB,
                                 uint8_t currentValue) {
  static const int8_t quadratureTable[16] = {0, 1, -1, 0,  -1, 0,  0, 1,
                                             1, 0, 0,  -1, 0,  -1, 1, 0};

  uint8_t index = ((lastAB & 0x03) << 2) | (currentAB & 0x03);
  int8_t direction = quadratureTable[index];

  if (direction == 1) {
    return (currentValue < MIDI_MAX) ? (currentValue + 1) : MIDI_MAX;
  } else if (direction == -1) {
    return (currentValue > 0) ? (currentValue - 1) : 0;
  }

  return currentValue;
}

uint16_t applyEma(uint16_t currentFiltered, uint16_t newRaw, uint8_t alpha) {
  uint32_t result = (uint32_t)alpha * newRaw +
                    (uint32_t)(EMA_SCALE - alpha) * currentFiltered;
  return (uint16_t)(result / EMA_SCALE);
}

} // namespace ControlReader
