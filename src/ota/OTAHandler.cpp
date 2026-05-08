#include "OTAHandler.h"
#include <Arduino.h>
#include <Update.h>

namespace OTAHandler {

static OTAState state = OTAState::IDLE;
static uint32_t totalFwSize = 0;
static uint32_t bytesWritten = 0;

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

  // O ESP32 Update library verifica internamente o MD5/hash
  // O CRC32 é uma verificação adicional do protocolo I2C
  (void)expectedCrc; // TODO: implementar verificação CRC32 do payload

  if (!Update.end(true)) {
    state = OTAState::ERROR;
    return false;
  }

  state = OTAState::COMPLETE;

  // Reinicia após OTA bem-sucedido
  delay(100);
  ESP.restart();

  return true; // Nunca alcançado
}

void abort() {
  if (state == OTAState::RECEIVING) {
    Update.abort();
  }
  state = OTAState::IDLE;
  totalFwSize = 0;
  bytesWritten = 0;
}

OTAState getState() { return state; }

uint8_t getProgress() {
  if (totalFwSize == 0)
    return 0;
  return (uint8_t)((uint32_t)bytesWritten * 100 / totalFwSize);
}

} // namespace OTAHandler
