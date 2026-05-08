#pragma once

#include "../config.h"
#include <cstdint>


enum class TipoControle : uint8_t {
  BOTAO = 0,
  POTENCIOMETRO = 1,
  SENSOR = 2,
  ENCODER = 3
};

struct ControleHW {
  const char *label; // Max 12 caracteres
  uint8_t gpio;      // Pino principal (ou pino A para encoder)
  TipoControle tipo;
  uint8_t ccPadrao;
  bool invertido;
  uint8_t gpioB; // Pino B para encoder (0 se não aplicável)
};

namespace HardwareMap {
// Definido pelo usuário conforme o hardware montado
constexpr ControleHW CONTROLES[] = {
    // {"Label", gpio, tipo, cc, invertido, gpioB}
    {"Pot1", 0, TipoControle::POTENCIOMETRO, 1, false, 0},
    {"Btn1", 1, TipoControle::BOTAO, 2, false, 0},
    {"Enc1", 2, TipoControle::ENCODER, 3, false, 3},
};

constexpr uint8_t NUM_CONTROLES = sizeof(CONTROLES) / sizeof(CONTROLES[0]);

// Funções auxiliares
constexpr const char *getLabel(uint8_t idx) { return CONTROLES[idx].label; }
constexpr uint8_t getGpio(uint8_t idx) { return CONTROLES[idx].gpio; }
constexpr TipoControle getTipo(uint8_t idx) { return CONTROLES[idx].tipo; }
constexpr bool isInvertido(uint8_t idx) { return CONTROLES[idx].invertido; }
constexpr bool isAnalogico(uint8_t idx) {
  return CONTROLES[idx].tipo == TipoControle::POTENCIOMETRO ||
         CONTROLES[idx].tipo == TipoControle::SENSOR;
}
constexpr uint8_t getGpioB(uint8_t idx) { return CONTROLES[idx].gpioB; }
} // namespace HardwareMap

static_assert(HardwareMap::NUM_CONTROLES <= MAX_CONTROLES,
              "Exceeded maximum number of controls");
