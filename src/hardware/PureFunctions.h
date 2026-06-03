#pragma once

// Funções puras de processamento — compiláveis tanto no target (ESP32) quanto
// em testes nativos. Fonte ÚNICA de verdade — eliminando duplicação entre
// ControlReader.cpp e test/shared/pure_functions.cpp.

#include "../config.h"
#include <cstdint>

namespace ControlReader {

// Tabela de decodificação de quadratura (compartilhada entre funções de
// encoder) Index = (lastAB << 2) | currentAB Valores: 0 = sem mudança, 1 = CW,
// -1 = CCW
inline constexpr int8_t QUADRATURE_TABLE[16] = {
    //  cur: 00  01  10  11
    /*prev 00*/ 0,  1,  -1, 0,
    /*prev 01*/ -1, 0,  0,  1,
    /*prev 10*/ 1,  0,  0,  -1,
    /*prev 11*/ 0,  -1, 1,  0};

// Mapeia valor ADC 12-bit [0, 4095] para MIDI [0, 127] com arredondamento
inline uint8_t mapAdcToMidi(uint16_t adcValue) {
  // Arredondamento: (value * 127 + 2047) / 4095 distribui melhor os 128 níveis
  return static_cast<uint8_t>(
      (static_cast<uint32_t>(adcValue) * MIDI_MAX + (ADC_MAX / 2)) / ADC_MAX);
}

// Retorna true se a diferença entre novo e último valor excede a deadzone
inline bool applyDeadzone(uint8_t newValue, uint8_t lastValue,
                          uint8_t deadzone) {
  uint8_t diff =
      (newValue > lastValue) ? (newValue - lastValue) : (lastValue - newValue);
  return diff > deadzone;
}

// Retorna o estado estável do botão aplicando debounce temporal
inline bool applyDebounce(bool reading, bool lastStable, uint32_t lastChangeMs,
                          uint32_t nowMs, uint16_t debounceMs) {
  if (reading != lastStable) {
    if ((nowMs - lastChangeMs) >= debounceMs) {
      return reading;
    }
  }
  return lastStable;
}

// Inverte valor MIDI: 0→127, 127→0
inline uint8_t invertValue(uint8_t value) { return MIDI_MAX - value; }

// Decodifica transição de encoder (sem aceleração)
inline uint8_t processEncoderTransition(uint8_t lastAB, uint8_t currentAB,
                                        uint8_t currentValue) {
  uint8_t index = ((lastAB & 0x03) << 2) | (currentAB & 0x03);
  int8_t direction = QUADRATURE_TABLE[index];

  if (direction == 1) {
    return (currentValue < MIDI_MAX) ? (currentValue + 1) : MIDI_MAX;
  } else if (direction == -1) {
    return (currentValue > 0) ? (currentValue - 1) : 0;
  }
  return currentValue;
}

// Decodifica transição de encoder com aceleração baseada em tempo
inline uint8_t processEncoderWithAccel(uint8_t lastAB, uint8_t currentAB,
                                       uint8_t currentValue,
                                       uint32_t elapsedMs) {
  uint8_t index = ((lastAB & 0x03) << 2) | (currentAB & 0x03);
  int8_t direction = QUADRATURE_TABLE[index];

  if (direction == 0)
    return currentValue;

  // Calcula step baseado na velocidade de rotação
  uint8_t step = 1;
  if (elapsedMs <= ENC_ACCEL_FAST_MS) {
    step = ENC_ACCEL_FAST_STEP;
  } else if (elapsedMs <= ENC_ACCEL_MED_MS) {
    step = ENC_ACCEL_MED_STEP;
  }

  if (direction == 1) {
    uint8_t newVal = currentValue + step;
    return (newVal > MIDI_MAX || newVal < currentValue) ? MIDI_MAX : newVal;
  } else {
    return (currentValue >= step) ? (currentValue - step) : 0;
  }
}

// EMA (Exponential Moving Average) em ponto fixo
// filtered = alpha/256 * newRaw + (256-alpha)/256 * currentFiltered
inline uint16_t applyEma(uint16_t currentFiltered, uint16_t newRaw,
                         uint8_t alpha) {
  uint32_t result = (uint32_t)alpha * newRaw +
                    (uint32_t)(EMA_SCALE - alpha) * currentFiltered;
  return (uint16_t)(result / EMA_SCALE);
}

} // namespace ControlReader
