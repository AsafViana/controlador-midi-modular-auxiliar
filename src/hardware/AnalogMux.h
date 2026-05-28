#pragma once

#include "../config.h"
#include <cstdint>

// Configuração de multiplexador analógico (CD4051 = 8ch, CD4067 = 16ch)
struct MuxConfig {
  uint8_t gpioSig;     // Pino ADC de sinal (saída do mux)
  uint8_t gpioS0;      // Pino de seleção S0
  uint8_t gpioS1;      // Pino de seleção S1
  uint8_t gpioS2;      // Pino de seleção S2
  uint8_t gpioS3;      // Pino de seleção S3 (0 se CD4051, usado se CD4067)
  uint8_t numChannels; // 8 para CD4051, 16 para CD4067
};

namespace AnalogMux {

// Máximo de multiplexadores suportados
constexpr uint8_t MAX_MUX = 2;

// Número de muxes configurados (alterar ao adicionar muxes)
constexpr uint8_t NUM_MUX = 0;

// Configuração dos muxes (definida pelo usuário)
// Descomentar e preencher ao usar multiplexadores, ajustar NUM_MUX acima.
// Exemplo para CD4051 no GPIO 5, seleção 6/7/10:
//   constexpr MuxConfig MUX_CONFIG[NUM_MUX] = {
//       {5, 6, 7, 10, 0, 8},
//   };

// Inicializa GPIOs dos multiplexadores
void init();

// Seleciona um canal do mux e lê o valor ADC
uint16_t readChannel(uint8_t muxIdx, uint8_t channel);

} // namespace AnalogMux
