/**
 * Property-based test: ModuleDescriptor serialization round-trip
 *
 * Feature: midi-extension-firmware, Property 5: ModuleDescriptor serialization
 * round-trip
 *
 * Validates: Requirements 4.2, 4.4, 4.6
 */

#include <rapidcheck.h>

#include <cstring>
#include <string>
#include <vector>

#include "../../src/hardware/HardwareMap.h"
#include "../../src/i2c/I2CSlave.h"

int main() {
  rc::check(
      "Serialization round-trip recovers original types, labels, and values",
      []() {
        // Generate random count of controls: 1 to 16
        auto count = *rc::gen::inRange<uint8_t>(1, 17);

        // Generate labels (store as strings to keep memory alive)
        std::vector<std::string> labels;
        std::vector<ControleHW> controls;
        std::vector<uint8_t> values;

        for (uint8_t i = 0; i < count; i++) {
          // Generate label: 1-12 alphanumeric characters
          auto labelLen = *rc::gen::inRange<int>(1, 13);
          auto label = *rc::gen::container<std::string>(
              labelLen, rc::gen::inRange<char>('a', 'z' + 1));
          labels.push_back(label);

          // Generate random TipoControle (0-3)
          auto tipo =
              static_cast<TipoControle>(*rc::gen::inRange<uint8_t>(0, 4));

          // Generate random value in [0, 127]
          auto value = *rc::gen::inRange<uint8_t>(0, 128);
          values.push_back(value);

          // Build ControleHW struct
          controls.push_back({labels.back().c_str(), 0, tipo, 0, false, 0, 0});
        }

        // Serialize
        uint8_t buffer[1 + 14 * 16] = {0};
        I2CSlave::serializeDescriptor(controls.data(), values.data(), count,
                                      buffer);

        // Deserialize and verify round-trip
        RC_ASSERT(buffer[0] == count);

        for (uint8_t i = 0; i < count; i++) {
          uint8_t *entry = &buffer[1 + i * 14];

          // Verify tipo
          RC_ASSERT(entry[0] == static_cast<uint8_t>(controls[i].tipo));

          // Verify label (12 bytes, zero-padded)
          char deserializedLabel[13] = {0};
          memcpy(deserializedLabel, &entry[1], 12);
          RC_ASSERT(std::string(deserializedLabel) == labels[i]);

          // Verify value
          RC_ASSERT(entry[13] == values[i]);
        }
      });

  return 0;
}
