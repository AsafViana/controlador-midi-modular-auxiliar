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
  uint8_t gpioB;      // Pino B para encoder (0 se não aplicável)
  uint8_t gpioSwitch; // Pino do push-button do encoder (0 se não aplicável)
};

namespace HardwareMap {
// Definido pelo usuário conforme o hardware montado
constexpr ControleHW CONTROLES[] = {
    // {"Label", gpio, tipo, cc, invertido, gpioB, gpioSwitch}
    {"Pot Aux", 0, TipoControle::POTENCIOMETRO, 1, false, 0, 0},
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
constexpr uint8_t getGpioSwitch(uint8_t idx) {
  return CONTROLES[idx].gpioSwitch;
}

// Calcula o número total de slots no buffer (controles + push-buttons de
// encoders)
constexpr uint8_t countTotalSlots(uint8_t idx = 0, uint8_t count = 0) {
  if (idx >= NUM_CONTROLES)
    return count;
  uint8_t extra = (CONTROLES[idx].tipo == TipoControle::ENCODER &&
                   CONTROLES[idx].gpioSwitch > 0)
                      ? 1
                      : 0;
  return countTotalSlots(idx + 1, count + 1 + extra);
}

constexpr uint8_t TOTAL_SLOTS = countTotalSlots();
} // namespace HardwareMap

static_assert(HardwareMap::NUM_CONTROLES <= MAX_CONTROLES,
              "Exceeded maximum number of controls");

static_assert(HardwareMap::TOTAL_SLOTS <= MAX_CONTROLES,
              "TOTAL_SLOTS (controls + encoder push-buttons) exceeds "
              "MAX_CONTROLES buffer size");

// --- Validação de GPIOs em tempo de compilação ---

// GPIOs ADC válidos no ESP32-C3: 0, 1, 2, 3, 4, 5
constexpr bool isValidAdcGpio(uint8_t gpio) { return gpio <= 5; }

// GPIOs digitais válidos no ESP32-C3: 0-10, 18-21
constexpr bool isValidDigitalGpio(uint8_t gpio) {
  return (gpio <= 10) || (gpio >= 18 && gpio <= 21);
}

// Valida um controle individual
constexpr bool validateControl(uint8_t idx) {
  if (HardwareMap::CONTROLES[idx].tipo == TipoControle::POTENCIOMETRO ||
      HardwareMap::CONTROLES[idx].tipo == TipoControle::SENSOR) {
    return isValidAdcGpio(HardwareMap::CONTROLES[idx].gpio);
  }
  if (HardwareMap::CONTROLES[idx].tipo == TipoControle::BOTAO) {
    return isValidDigitalGpio(HardwareMap::CONTROLES[idx].gpio);
  }
  if (HardwareMap::CONTROLES[idx].tipo == TipoControle::ENCODER) {
    bool valid = isValidDigitalGpio(HardwareMap::CONTROLES[idx].gpio) &&
                 isValidDigitalGpio(HardwareMap::CONTROLES[idx].gpioB);
    // Valida gpioSwitch se definido (> 0)
    if (HardwareMap::CONTROLES[idx].gpioSwitch > 0) {
      valid =
          valid && isValidDigitalGpio(HardwareMap::CONTROLES[idx].gpioSwitch);
    }
    return valid;
  }
  return false;
}

// Valida todos os controles recursivamente em constexpr
constexpr bool validateAllControls(uint8_t idx = 0) {
  if (idx >= HardwareMap::NUM_CONTROLES)
    return true;
  return validateControl(idx) && validateAllControls(idx + 1);
}

static_assert(validateAllControls(),
              "Invalid GPIO configuration detected! "
              "ADC controls (POT/SENSOR) must use GPIO 0-5. "
              "Digital controls (BUTTON/ENCODER) must use GPIO 0-10 or 18-21.");

// --- Validação de CC MIDI duplicado em tempo de compilação ---

constexpr bool hasDuplicateCC(uint8_t idx = 0) {
  if (idx >= HardwareMap::NUM_CONTROLES)
    return false;
  for (uint8_t j = idx + 1; j < HardwareMap::NUM_CONTROLES; j++) {
    if (HardwareMap::CONTROLES[idx].ccPadrao ==
        HardwareMap::CONTROLES[j].ccPadrao)
      return true;
  }
  return hasDuplicateCC(idx + 1);
}

static_assert(!hasDuplicateCC(),
              "Duplicate MIDI CC detected! Each control must have a unique CC "
              "number within the same module.");
