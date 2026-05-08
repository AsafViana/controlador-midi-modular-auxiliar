// Pure function implementations for native testing (no Arduino.h dependency)
#include "../../src/config.h"
#include "../../src/hardware/ControlReader.h"
#include "../../src/hardware/HardwareMap.h"
#include "../../src/i2c/I2CSlave.h"
#include <cstring>

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

namespace I2CSlave {

void serializeDescriptor(const ControleHW *controls, const uint8_t *values,
                         uint8_t count, uint8_t *outBuffer) {
  // First byte: number of controls
  outBuffer[0] = count;

  // For each control: tipo(1) + label(12, zero-padded) + valor(1) = 14 bytes
  for (uint8_t i = 0; i < count; i++) {
    uint8_t *entry = &outBuffer[1 + (i * 14)];

    // tipo: 1 byte (cast from TipoControle enum)
    entry[0] = static_cast<uint8_t>(controls[i].tipo);

    // label: 12 bytes, zero-padded
    memset(&entry[1], 0, 12);
    strncpy(reinterpret_cast<char *>(&entry[1]), controls[i].label, 12);

    // valor: 1 byte
    entry[13] = values[i];
  }
}

} // namespace I2CSlave
