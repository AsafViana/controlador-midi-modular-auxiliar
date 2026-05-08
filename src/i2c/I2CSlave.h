#pragma once

#include <cstdint>

// Forward declare ControleHW
struct ControleHW;

namespace I2CSlave {
// Inicializa Wire no modo escravo e registra callbacks
void init();

// Verifica se houve atividade I2C recente (desde última chamada)
bool hasRecentActivity();

// Pure serialization function (testable without hardware)
void serializeDescriptor(const ControleHW *controls, const uint8_t *values,
                         uint8_t count, uint8_t *outBuffer);
} // namespace I2CSlave
