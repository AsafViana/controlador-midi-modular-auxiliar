#pragma once

#include "../config.h"
#include <cstdint>

namespace PersistentConfig {

// Estrutura de configuração por canal
struct ChannelConfig {
  uint8_t deadzone;  // Deadzone para analógicos (0-127)
  uint16_t debounce; // Debounce para botões (ms)
  bool invertido;    // Inversão de valor
};

// Inicializa configuração (carrega da NVS ou usa padrões)
void init();

// Obtém configuração de um canal
ChannelConfig getConfig(uint8_t channel);

// Define configuração de um canal e salva na NVS
// Formato do payload I2C: [channel(1), deadzone(1), debounce_hi(1),
// debounce_lo(1), invertido(1)]
void setConfig(uint8_t channel, uint8_t deadzone, uint16_t debounce,
               bool invertido);

// Aplica configuração recebida via I2C (parse do payload)
void applyFromI2C(const uint8_t *data, uint8_t len);

} // namespace PersistentConfig
