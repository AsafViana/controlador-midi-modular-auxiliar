#pragma once

#include "../config.h"
#include <cstdint>

namespace OTAHandler {

enum class OTAState : uint8_t {
  IDLE = 0,
  RECEIVING = 1,
  COMPLETE = 2,
  ERROR = 3
};

// Inicia processo OTA com tamanho total do firmware
bool begin(uint32_t totalSize);

// Recebe um bloco de dados
bool writeBlock(uint32_t offset, const uint8_t *data, uint16_t len);

// Finaliza OTA com verificação CRC32
bool end(uint32_t expectedCrc);

// Aborta OTA em andamento
void abort();

// Retorna estado atual
OTAState getState();

// Retorna progresso (0-100)
uint8_t getProgress();

} // namespace OTAHandler
