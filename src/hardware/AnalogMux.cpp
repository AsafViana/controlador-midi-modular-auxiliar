#include "AnalogMux.h"
#include <Arduino.h>

namespace AnalogMux {

void init() {
  // Sem mux configurado (NUM_MUX == 0): nada a inicializar
}

uint16_t readChannel(uint8_t muxIdx, uint8_t channel) {
  // Sem mux configurado (NUM_MUX == 0): sempre retorna 0
  (void)muxIdx;
  (void)channel;
  return 0;
}

} // namespace AnalogMux
