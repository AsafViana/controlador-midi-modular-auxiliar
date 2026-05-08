/**
 * Property-based test: Debounce rejects unstable signals
 *
 * Feature: midi-extension-firmware, Property 3: Debounce rejects unstable
 * signals
 *
 * Validates: Requirements 3.3
 */

#include <rapidcheck.h>

#include "../../src/hardware/ControlReader.h"

int main() {
  rc::check(
      "Debounce does not transition when time elapsed is less than debounce "
      "period",
      []() {
        auto reading = *rc::gen::arbitrary<bool>();
        auto lastStable = !reading; // Ensure reading != lastStable

        auto debounceMs = *rc::gen::inRange<uint16_t>(1, 1000);
        auto lastChangeMs = *rc::gen::inRange<uint32_t>(0, UINT32_MAX - 1000);

        // Generate nowMs such that (nowMs - lastChangeMs) < debounceMs
        auto elapsed = *rc::gen::inRange<uint32_t>(0, debounceMs);
        uint32_t nowMs = lastChangeMs + elapsed;

        bool result = ControlReader::applyDebounce(
            reading, lastStable, lastChangeMs, nowMs, debounceMs);
        RC_ASSERT(result == lastStable);
      });

  rc::check(
      "Debounce transitions when input stable for at least debounce period",
      []() {
        auto reading = *rc::gen::arbitrary<bool>();
        auto lastStable = !reading; // Ensure reading != lastStable

        auto debounceMs = *rc::gen::inRange<uint16_t>(1, 1000);
        auto lastChangeMs = *rc::gen::inRange<uint32_t>(0, UINT32_MAX - 2000);

        // Generate nowMs such that (nowMs - lastChangeMs) >= debounceMs
        auto extra = *rc::gen::inRange<uint32_t>(0, 1000);
        uint32_t nowMs = lastChangeMs + debounceMs + extra;

        bool result = ControlReader::applyDebounce(
            reading, lastStable, lastChangeMs, nowMs, debounceMs);
        RC_ASSERT(result == reading);
      });

  rc::check("Debounce returns lastStable when reading equals lastStable", []() {
    auto reading = *rc::gen::arbitrary<bool>();
    auto lastStable = reading; // reading == lastStable

    auto debounceMs = *rc::gen::inRange<uint16_t>(0, 1000);
    auto lastChangeMs = *rc::gen::inRange<uint32_t>(0, UINT32_MAX - 2000);
    auto nowMs = *rc::gen::inRange<uint32_t>(lastChangeMs, lastChangeMs + 2000);

    bool result = ControlReader::applyDebounce(reading, lastStable,
                                               lastChangeMs, nowMs, debounceMs);
    RC_ASSERT(result == lastStable);
  });

  return 0;
}
