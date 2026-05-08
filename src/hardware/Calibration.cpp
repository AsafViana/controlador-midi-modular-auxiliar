#include "Calibration.h"
#include "HardwareMap.h"
#include <Preferences.h>

namespace Calibration {

static ChannelCal channels[MAX_CONTROLES];
static bool calibrating = false;
static uint8_t calChannel = 0;
static Preferences prefs;

void init() {
  // Valores padrão: full range
  for (uint8_t i = 0; i < MAX_CONTROLES; i++) {
    channels[i] = {0, ADC_MAX};
  }

  // Carrega da NVS
  prefs.begin("cal", true); // read-only
  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    if (HardwareMap::isAnalogico(i)) {
      char keyMin[8], keyMax[8];
      snprintf(keyMin, sizeof(keyMin), "mn%d", i);
      snprintf(keyMax, sizeof(keyMax), "mx%d", i);
      channels[i].adcMin = prefs.getUShort(keyMin, 0);
      channels[i].adcMax = prefs.getUShort(keyMax, ADC_MAX);
    }
  }
  prefs.end();
}

void startCalibration(uint8_t channel) {
  if (channel >= HardwareMap::NUM_CONTROLES)
    return;
  if (!HardwareMap::isAnalogico(channel))
    return;

  calibrating = true;
  calChannel = channel;
  // Reset min/max para capturar novos extremos
  channels[channel].adcMin = ADC_MAX;
  channels[channel].adcMax = 0;
}

void stopCalibration() {
  if (!calibrating)
    return;

  // Validação: min deve ser menor que max com margem mínima
  if (channels[calChannel].adcMin >= channels[calChannel].adcMax ||
      (channels[calChannel].adcMax - channels[calChannel].adcMin) < 100) {
    // Calibração inválida, restaura padrão
    channels[calChannel] = {0, ADC_MAX};
  }

  // Salva na NVS
  prefs.begin("cal", false); // read-write
  char keyMin[8], keyMax[8];
  snprintf(keyMin, sizeof(keyMin), "mn%d", calChannel);
  snprintf(keyMax, sizeof(keyMax), "mx%d", calChannel);
  prefs.putUShort(keyMin, channels[calChannel].adcMin);
  prefs.putUShort(keyMax, channels[calChannel].adcMax);
  prefs.end();

  calibrating = false;
}

void feedRawValue(uint8_t channel, uint16_t adcRaw) {
  if (!calibrating || channel != calChannel)
    return;

  if (adcRaw < channels[channel].adcMin) {
    channels[channel].adcMin = adcRaw;
  }
  if (adcRaw > channels[channel].adcMax) {
    channels[channel].adcMax = adcRaw;
  }
}

uint16_t applyCal(uint16_t adcRaw, uint16_t adcMin, uint16_t adcMax) {
  if (adcMin >= adcMax)
    return adcRaw; // Sem calibração válida

  // Clamp ao range calibrado
  if (adcRaw <= adcMin)
    return 0;
  if (adcRaw >= adcMax)
    return ADC_MAX;

  // Mapeia [adcMin, adcMax] → [0, ADC_MAX]
  uint32_t scaled = (uint32_t)(adcRaw - adcMin) * ADC_MAX / (adcMax - adcMin);
  return (uint16_t)scaled;
}

ChannelCal getChannelCal(uint8_t channel) {
  if (channel >= MAX_CONTROLES)
    return {0, ADC_MAX};
  return channels[channel];
}

bool isCalibrating() { return calibrating; }

uint8_t getCalibratingChannel() { return calChannel; }

} // namespace Calibration
