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

// Configuração dos muxes (definida pelo usuário)
// Deixar vazio se não usar mux
constexpr MuxConfig MUX_CONFIG[] = {
    // Exemplo: {gpioSig, S0, S1, S2, S3, numChannels}
    // {"5, 6, 7, 8, 0, 8},  // CD4051 no GPIO 5, seleção 6/7/8
};

constexpr uint8_t NUM_MUX = sizeof(MUX_CONFIG) / sizeof(MUX_CONFIG[0]);

// Inicializa GPIOs dos multiplexadores
void init();

// Seleciona um canal do mux e lê o valor ADC
uint16_t readChannel(uint8_t muxIdx, uint8_t channel);

} // namespace AnalogMux
