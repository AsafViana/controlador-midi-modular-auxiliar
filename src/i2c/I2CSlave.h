#pragma once

#include <cstdint>

// Forward declare ControleHW
struct ControleHW;

namespace I2CSlave {
// Inicializa Wire no modo escravo e registra callbacks
void init();

// Pure serialization function (testable without hardware)
// Serializes the module descriptor into outBuffer.
// Format: numControles(1) + [tipo(1) + label(12, zero-padded) + valor(1)] per
// control = 14 bytes per control. Total: 1 + (14 * count) bytes.
void serializeDescriptor(const ControleHW *controls, const uint8_t *values,
                         uint8_t count, uint8_t *outBuffer);
} // namespace I2CSlave
