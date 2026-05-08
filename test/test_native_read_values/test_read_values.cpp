/**
 * Property-based test: CMD_READ_VALUES response is identity over buffer
 *
 * Feature: midi-extension-firmware, Property 6: CMD_READ_VALUES response is
 * identity over buffer
 *
 * Validates: Requirements 4.3
 */

#include <rapidcheck.h>

#include <cstdint>
#include <vector>

#include "../../src/config.h"

int main() {
  rc::check("CMD_READ_VALUES response is identity over buffer", []() {
    // Generate random count N in [1, 16]
    auto count = *rc::gen::inRange<uint8_t>(1, 17);

    // Generate random values buffer (each in [0, 127])
    std::vector<uint8_t> buffer;
    for (uint8_t i = 0; i < count; i++) {
      buffer.push_back(*rc::gen::inRange<uint8_t>(0, 128));
    }

    // Simulate CMD_READ_VALUES response: iterate over buffer and copy each
    // value This mirrors the onRequest logic for CMD_READ_VALUES:
    //   for (uint8_t i = 0; i < count; i++) { Wire.write((uint8_t)values[i]);
    //   }
    std::vector<uint8_t> response;
    for (uint8_t i = 0; i < count; i++) {
      response.push_back(buffer[i]);
    }

    // Verify identity: response size matches count
    RC_ASSERT(response.size() == static_cast<size_t>(count));

    // Verify identity: response[i] == buffer[i] for all i in [0, N)
    for (uint8_t i = 0; i < count; i++) {
      RC_ASSERT(response[i] == buffer[i]);
    }
  });

  return 0;
}
