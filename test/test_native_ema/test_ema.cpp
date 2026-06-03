/**
 * Property-based test: EMA filter properties
 *
 * Feature: midi-extension-firmware
 *
 * Validates: EMA stays bounded, interpolates between current and new value
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check(
      "EMA output is always between min and max of inputs (inclusive)", []() {
        auto currentFiltered = *rc::gen::inRange<uint16_t>(0, 4096);
        auto newRaw = *rc::gen::inRange<uint16_t>(0, 4096);
        auto alpha = *rc::gen::inRange<uint16_t>(0, 256);

        uint16_t result = ControlReader::applyEma(currentFiltered, newRaw,
                                                  static_cast<uint8_t>(alpha));

        uint16_t lo = (currentFiltered < newRaw) ? currentFiltered : newRaw;
        uint16_t hi = (currentFiltered > newRaw) ? currentFiltered : newRaw;

        RC_ASSERT(result >= lo);
        RC_ASSERT(result <= hi);
      });

  rc::check("EMA with alpha=0 returns currentFiltered (100% old)", []() {
    auto currentFiltered = *rc::gen::inRange<uint16_t>(0, 4096);
    auto newRaw = *rc::gen::inRange<uint16_t>(0, 4096);

    uint16_t result = ControlReader::applyEma(currentFiltered, newRaw, 0);
    RC_ASSERT(result == currentFiltered);
  });

  rc::check("EMA with alpha=EMA_SCALE has result close to newRaw", []() {
    auto currentFiltered = *rc::gen::inRange<uint16_t>(0, 4096);
    auto newRaw = *rc::gen::inRange<uint16_t>(0, 4096);

    // alpha = 256 means EMA_SCALE - alpha = 0, so result = newRaw
    // But alpha is uint8_t so max is 255; with 255:
    // result = (255*newRaw + 1*currentFiltered) / 256
    // Close to newRaw but not exact (off by at most 1/256 of range)
    uint16_t result = ControlReader::applyEma(currentFiltered, newRaw, 255);

    // Should be very close to newRaw (within ±16 for 12-bit values)
    int32_t diff = (int32_t)result - (int32_t)newRaw;
    if (diff < 0)
      diff = -diff;
    RC_ASSERT(diff <= 16);
  });

  rc::check("EMA is idempotent when inputs are equal", []() {
    auto value = *rc::gen::inRange<uint16_t>(0, 4096);
    auto alpha = *rc::gen::inRange<uint16_t>(0, 256);

    uint16_t result =
        ControlReader::applyEma(value, value, static_cast<uint8_t>(alpha));
    RC_ASSERT(result == value);
  });

  rc::check("EMA output never exceeds ADC_MAX for valid inputs", []() {
    auto currentFiltered = *rc::gen::inRange<uint16_t>(0, 4096);
    auto newRaw = *rc::gen::inRange<uint16_t>(0, 4096);
    auto alpha = *rc::gen::inRange<uint16_t>(0, 256);

    uint16_t result = ControlReader::applyEma(currentFiltered, newRaw,
                                              static_cast<uint8_t>(alpha));
    RC_ASSERT(result <= 4095);
  });

  return 0;
}
