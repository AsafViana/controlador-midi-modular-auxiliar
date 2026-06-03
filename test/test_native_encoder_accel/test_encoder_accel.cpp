/**
 * Property-based test: Encoder with acceleration properties
 *
 * Feature: midi-extension-firmware
 *
 * Validates: Acceleration respects bounds, speed tiers produce correct steps
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check("Encoder with accel always stays in [0, 127] for any transition",
            []() {
              auto startValue = *rc::gen::inRange<uint8_t>(0, 128);
              auto lastAB = *rc::gen::inRange<uint8_t>(0, 4);
              auto currentAB = *rc::gen::inRange<uint8_t>(0, 4);
              auto elapsedMs = *rc::gen::inRange<uint32_t>(0, 10000);

              uint8_t result = ControlReader::processEncoderWithAccel(
                  lastAB, currentAB, startValue, elapsedMs);
              RC_ASSERT(result <= 127);
            });

  rc::check("Fast rotation (<=50ms) increments by ENC_ACCEL_FAST_STEP", []() {
    auto startValue = *rc::gen::inRange<uint8_t>(10, 118);
    auto elapsedMs = *rc::gen::inRange<uint32_t>(1, 51);

    // CW transition: 00 -> 01 (direction = +1)
    uint8_t result = ControlReader::processEncoderWithAccel(
        0b00, 0b01, startValue, elapsedMs);
    // Expected: startValue + ENC_ACCEL_FAST_STEP (8), clamped at 127
    uint8_t expected = startValue + 8;
    if (expected > 127)
      expected = 127;
    RC_ASSERT(result == expected);
  });

  rc::check("Medium rotation (51-150ms) increments by ENC_ACCEL_MED_STEP",
            []() {
              auto startValue = *rc::gen::inRange<uint8_t>(10, 124);
              auto elapsedMs = *rc::gen::inRange<uint32_t>(51, 151);

              // CW transition: 00 -> 01 (direction = +1)
              uint8_t result = ControlReader::processEncoderWithAccel(
                  0b00, 0b01, startValue, elapsedMs);
              uint8_t expected = startValue + 4;
              if (expected > 127)
                expected = 127;
              RC_ASSERT(result == expected);
            });

  rc::check("Slow rotation (>150ms) increments by 1", []() {
    auto startValue = *rc::gen::inRange<uint8_t>(0, 127);
    auto elapsedMs = *rc::gen::inRange<uint32_t>(151, 10000);

    // CW transition: 00 -> 01 (direction = +1)
    uint8_t result = ControlReader::processEncoderWithAccel(
        0b00, 0b01, startValue, elapsedMs);
    uint8_t expected = (startValue < 127) ? (startValue + 1) : 127;
    RC_ASSERT(result == expected);
  });

  rc::check("Fast CCW from low value clamps at 0 (no underflow)", []() {
    auto startValue = *rc::gen::inRange<uint8_t>(0, 8);
    auto elapsedMs = *rc::gen::inRange<uint32_t>(1, 51);

    // CCW transition: 00 -> 10 (direction = -1)
    uint8_t result = ControlReader::processEncoderWithAccel(
        0b00, 0b10, startValue, elapsedMs);
    RC_ASSERT(result == 0);
  });

  rc::check("Fast CW from high value clamps at 127 (no overflow)", []() {
    auto startValue = *rc::gen::inRange<uint8_t>(120, 128);
    auto elapsedMs = *rc::gen::inRange<uint32_t>(1, 51);

    // CW transition: 00 -> 01 (direction = +1)
    uint8_t result = ControlReader::processEncoderWithAccel(
        0b00, 0b01, startValue, elapsedMs);
    RC_ASSERT(result == 127);
  });

  rc::check("Invalid transition returns current value regardless of speed",
            []() {
              auto startValue = *rc::gen::inRange<uint8_t>(0, 128);
              auto elapsedMs = *rc::gen::inRange<uint32_t>(0, 10000);

              // Invalid transition: 00 -> 11 (direction = 0)
              uint8_t result = ControlReader::processEncoderWithAccel(
                  0b00, 0b11, startValue, elapsedMs);
              RC_ASSERT(result == startValue);
            });

  return 0;
}
