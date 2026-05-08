#pragma once

#include "../config.h"
#include <cstdint>

namespace Calibration {

struct ChannelCal {
  uint16_t adcMin;
  uint16_t adcMax;
};

// Inicializa calibração (carrega da NVS ou usa valores padrão)
void init();

// Inicia modo de calibração para um canal específico
void startCalibration(uint8_t channel);

// Finaliza calibração e salva na NVS
void stopCalibration();

// Atualiza calibração com nova leitura ADC (chamado durante modo calibração)
void feedRawValue(uint8_t channel, uint16_t adcRaw);

// Aplica calibração: mapeia [adcMin, adcMax] → [0, ADC_MAX]
// Pure function, testável sem hardware
uint16_t applyCal(uint16_t adcRaw, uint16_t adcMin, uint16_t adcMax);

// Retorna calibração de um canal
ChannelCal getChannelCal(uint8_t channel);

// Verifica se está em modo calibração
bool isCalibrating();

// Retorna o canal sendo calibrado
uint8_t getCalibratingChannel();

} // namespace Calibration
