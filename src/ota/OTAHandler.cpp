#include "OTAHandler.h"
#include <Arduino.h>
#include <Update.h>

namespace OTAHandler {

static OTAState state = OTAState::IDLE;
static uint32_t totalFwSize = 0;
static uint32_t bytesWritten = 0;
static uint32_t runningCrc = 0xFFFFFFFF;

// Tabela CRC32 (polinômio 0xEDB88320, padrão Ethernet/ZIP)
static uint32_t crc32Update(uint32_t crc, const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool begin(uint32_t totalSize) {
  if (state == OTAState::RECEIVING) {
    abort();
  }

  if (totalSize == 0 || totalSize > 0x200000) { // Max 2MB
    state = OTAState::ERROR;
    return false;
  }

  if (!Update.begin(totalSize)) {
    state = OTAState::ERROR;
    return false;
  }

  totalFwSize = totalSize;
  bytesWritten = 0;
  runningCrc = 0xFFFFFFFF;
  state = OTAState::RECEIVING;
  return true;
}

bool writeBlock(uint32_t offset, const uint8_t *data, uint16_t len) {
  if (state != OTAState::RECEIVING)
    return false;
  if (offset != bytesWritten)
    return false; // Blocos devem ser sequenciais

  size_t written = Update.write((uint8_t *)data, (size_t)len);
  if (written != (size_t)len) {
    state = OTAState::ERROR;
    Update.abort();
    return false;
  }

  // Atualiza CRC32 incremental
  runningCrc = crc32Update(runningCrc, data, len);

  bytesWritten += len;
  return true;
}

bool end(uint32_t expectedCrc) {
  if (state != OTAState::RECEIVING)
    return false;

  if (bytesWritten != totalFwSize) {
    state = OTAState::ERROR;
    Update.abort();
    return false;
  }

  // Finaliza CRC32 (XOR final)
  uint32_t finalCrc = runningCrc ^ 0xFFFFFFFF;

  // Verifica CRC32 do payload transmitido via I2C
  if (finalCrc != expectedCrc) {
    state = OTAState::ERROR;
    Update.abort();
    return false;
  }

  if (!Update.end(true)) {
    state = OTAState::ERROR;
    return false;
  }

  state = OTAState::COMPLETE;

  // Restart será feito pelo loop principal via isOtaRestartPending()
  return true;
}

void abort() {
  if (state == OTAState::RECEIVING) {
    Update.abort();
  }
  state = OTAState::IDLE;
  totalFwSize = 0;
  bytesWritten = 0;
  runningCrc = 0xFFFFFFFF;
}

OTAState getState() { return state; }

uint8_t getProgress() {
  if (totalFwSize == 0)
    return 0;
  return (uint8_t)((uint32_t)bytesWritten * 100 / totalFwSize);
}

} // namespace OTAHandler
