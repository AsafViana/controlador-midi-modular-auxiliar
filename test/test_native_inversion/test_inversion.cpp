/**
 * Property-based test: Inversion is self-inverse
 *
 * Feature: midi-extension-firmware, Property 4: Inversion is self-inverse
 *
 * Validates: Requirements 3.5
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check("Inversion is self-inverse: invertValue(invertValue(v)) == v",
            []() {
              auto v = *rc::gen::inRange<uint8_t>(0, 128);
              uint8_t inverted = ControlReader::invertValue(v);
              uint8_t doubleInverted = ControlReader::invertValue(inverted);
              RC_ASSERT(doubleInverted == v);
            });

  rc::check("Inversion equals 127 minus value", []() {
    auto v = *rc::gen::inRange<uint8_t>(0, 128);
    uint8_t result = ControlReader::invertValue(v);
    RC_ASSERT(result == static_cast<uint8_t>(127 - v));
  });

  return 0;
}
