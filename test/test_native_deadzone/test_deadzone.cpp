/**
 * Property-based test: Deadzone filters noise below threshold
 *
 * Feature: midi-extension-firmware, Property 2: Deadzone filters noise below
 * threshold
 *
 * Validates: Requirements 3.2
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check("Deadzone returns false when difference is within threshold", []() {
    auto lastValue = *rc::gen::inRange<uint8_t>(0, 128);
    auto deadzone = *rc::gen::inRange<uint8_t>(1, 128);

    // Generate newValue such that |newValue - lastValue| <= deadzone
    int low = static_cast<int>(lastValue) - static_cast<int>(deadzone);
    int high = static_cast<int>(lastValue) + static_cast<int>(deadzone);
    if (low < 0)
      low = 0;
    if (high > 127)
      high = 127;

    auto newValue = *rc::gen::inRange<uint8_t>(static_cast<uint8_t>(low),
                                               static_cast<uint8_t>(high + 1));

    RC_ASSERT(!ControlReader::applyDeadzone(newValue, lastValue, deadzone));
  });

  rc::check("Deadzone returns true when difference exceeds threshold", []() {
    auto lastValue = *rc::gen::inRange<uint8_t>(0, 128);
    auto deadzone = *rc::gen::inRange<uint8_t>(0, 128);

    // Generate newValue such that |newValue - lastValue| > deadzone
    // Calculate valid ranges: [0, lastValue - deadzone - 1] or
    //                         [lastValue + deadzone + 1, 127]
    int lowEnd = static_cast<int>(lastValue) - static_cast<int>(deadzone) - 1;
    int highStart =
        static_cast<int>(lastValue) + static_cast<int>(deadzone) + 1;

    bool hasLowRange = lowEnd >= 0;
    bool hasHighRange = highStart <= 127;

    // Discard if no valid value exists that exceeds the deadzone
    RC_PRE(hasLowRange || hasHighRange);

    uint8_t newValue;
    if (hasLowRange && hasHighRange) {
      // Pick from either range
      auto pickLow = *rc::gen::arbitrary<bool>();
      if (pickLow) {
        newValue =
            *rc::gen::inRange<uint8_t>(0, static_cast<uint8_t>(lowEnd + 1));
      } else {
        newValue =
            *rc::gen::inRange<uint8_t>(static_cast<uint8_t>(highStart), 128);
      }
    } else if (hasLowRange) {
      newValue =
          *rc::gen::inRange<uint8_t>(0, static_cast<uint8_t>(lowEnd + 1));
    } else {
      newValue =
          *rc::gen::inRange<uint8_t>(static_cast<uint8_t>(highStart), 128);
    }

    RC_ASSERT(ControlReader::applyDeadzone(newValue, lastValue, deadzone));
  });

  return 0;
}
