#include "AnalogMux.h"
#include <Arduino.h>

namespace AnalogMux {

void init() {
  for (uint8_t m = 0; m < NUM_MUX; m++) {
    const auto &mux = MUX_CONFIG[m];
    pinMode(mux.gpioSig, INPUT);
    pinMode(mux.gpioS0, OUTPUT);
    pinMode(mux.gpioS1, OUTPUT);
    pinMode(mux.gpioS2, OUTPUT);
    if (mux.gpioS3 > 0) {
      pinMode(mux.gpioS3, OUTPUT);
    }
  }
}

uint16_t readChannel(uint8_t muxIdx, uint8_t channel) {
  if (muxIdx >= NUM_MUX)
    return 0;

  const auto &mux = MUX_CONFIG[muxIdx];
  if (channel >= mux.numChannels)
    return 0;

  // Seleciona canal via pinos de seleção
  digitalWrite(mux.gpioS0, (channel >> 0) & 0x01);
  digitalWrite(mux.gpioS1, (channel >> 1) & 0x01);
  digitalWrite(mux.gpioS2, (channel >> 2) & 0x01);
  if (mux.gpioS3 > 0) {
    digitalWrite(mux.gpioS3, (channel >> 3) & 0x01);
  }

  // Pequeno delay para estabilização do mux (~1us é suficiente)
  delayMicroseconds(2);

  return analogRead(mux.gpioSig);
}

} // namespace AnalogMux
