/**
 * Property-based test: ADC to MIDI mapping preserves range and monotonicity
 *
 * Feature: midi-extension-firmware, Property 1: ADC to MIDI mapping preserves
 * range and monotonicity
 *
 * Validates: Requirements 3.4
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check("ADC to MIDI output is always in range [0, 127]", []() {
    auto adcValue = *rc::gen::inRange<uint16_t>(0, 4096);
    uint8_t result = ControlReader::mapAdcToMidi(adcValue);
    RC_ASSERT(result <= 127);
  });

  rc::check("ADC to MIDI mapping is monotonically non-decreasing", []() {
    auto a = *rc::gen::inRange<uint16_t>(0, 4096);
    auto b = *rc::gen::inRange<uint16_t>(0, 4096);
    if (a > b)
      std::swap(a, b);
    RC_ASSERT(ControlReader::mapAdcToMidi(a) <= ControlReader::mapAdcToMidi(b));
  });

  return 0;
}
