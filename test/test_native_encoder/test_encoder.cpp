/**
 * Property-based test: Encoder accumulation is bounded
 *
 * Feature: midi-extension-firmware, Property 7: Encoder accumulation is bounded
 *
 * Validates: Requirements 7.2, 7.3
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check(
      "Encoder value stays in [0, 127] for any sequence of transitions", []() {
        auto startValue = *rc::gen::inRange<uint8_t>(0, 128);
        auto numTransitions = *rc::gen::inRange<int>(1, 101);

        uint8_t value = startValue;
        uint8_t lastAB = *rc::gen::inRange<uint8_t>(0, 4);

        for (int t = 0; t < numTransitions; t++) {
          auto currentAB = *rc::gen::inRange<uint8_t>(0, 4);
          value =
              ControlReader::processEncoderTransition(lastAB, currentAB, value);
          RC_ASSERT(value <= 127);
          lastAB = currentAB;
        }
      });

  rc::check("CW from any value v produces min(v+1, 127)", []() {
    auto v = *rc::gen::inRange<uint8_t>(0, 128);
    // CW transition: 00 -> 01
    uint8_t result = ControlReader::processEncoderTransition(0b00, 0b01, v);
    uint8_t expected = (v < 127) ? (v + 1) : 127;
    RC_ASSERT(result == expected);
  });

  rc::check("CCW from any value v produces max(v-1, 0)", []() {
    auto v = *rc::gen::inRange<uint8_t>(0, 128);
    // CCW transition: 00 -> 10
    uint8_t result = ControlReader::processEncoderTransition(0b00, 0b10, v);
    uint8_t expected = (v > 0) ? (v - 1) : 0;
    RC_ASSERT(result == expected);
  });

  return 0;
}
