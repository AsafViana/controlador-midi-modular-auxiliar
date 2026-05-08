#include "PersistentConfig.h"
#include "HardwareMap.h"
#include <Preferences.h>

namespace PersistentConfig {

static ChannelConfig configs[MAX_CONTROLES];
static Preferences prefs;

void init() {
  // Valores padrão
  for (uint8_t i = 0; i < MAX_CONTROLES; i++) {
    configs[i] = {DEADZONE, DEBOUNCE_MS, false};
  }

  // Carrega da NVS
  prefs.begin("cfg", true); // read-only
  for (uint8_t i = 0; i < HardwareMap::NUM_CONTROLES; i++) {
    char keyDz[8], keyDb[8], keyInv[8];
    snprintf(keyDz, sizeof(keyDz), "dz%d", i);
    snprintf(keyDb, sizeof(keyDb), "db%d", i);
    snprintf(keyInv, sizeof(keyInv), "iv%d", i);

    configs[i].deadzone = prefs.getUChar(keyDz, DEADZONE);
    configs[i].debounce = prefs.getUShort(keyDb, DEBOUNCE_MS);
    configs[i].invertido = prefs.getBool(keyInv, HardwareMap::isInvertido(i));
  }
  prefs.end();
}

ChannelConfig getConfig(uint8_t channel) {
  if (channel >= MAX_CONTROLES)
    return {DEADZONE, DEBOUNCE_MS, false};
  return configs[channel];
}

void setConfig(uint8_t channel, uint8_t deadzone, uint16_t debounce,
               bool invertido) {
  if (channel >= HardwareMap::NUM_CONTROLES)
    return;

  configs[channel].deadzone = deadzone;
  configs[channel].debounce = debounce;
  configs[channel].invertido = invertido;

  // Salva na NVS
  prefs.begin("cfg", false); // read-write
  char keyDz[8], keyDb[8], keyInv[8];
  snprintf(keyDz, sizeof(keyDz), "dz%d", channel);
  snprintf(keyDb, sizeof(keyDb), "db%d", channel);
  snprintf(keyInv, sizeof(keyInv), "iv%d", channel);

  prefs.putUChar(keyDz, deadzone);
  prefs.putUShort(keyDb, debounce);
  prefs.putBool(keyInv, invertido);
  prefs.end();
}

void applyFromI2C(const uint8_t *data, uint8_t len) {
  // Formato: [channel(1), deadzone(1), debounce_hi(1), debounce_lo(1),
  // invertido(1)]
  if (len < 5)
    return;

  uint8_t channel = data[0];
  uint8_t deadzone = data[1];
  uint16_t debounce = ((uint16_t)data[2] << 8) | data[3];
  bool invertido = data[4] != 0;

  setConfig(channel, deadzone, debounce, invertido);
}

} // namespace PersistentConfig
