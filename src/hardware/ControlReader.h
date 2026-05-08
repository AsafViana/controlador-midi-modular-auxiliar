#pragma once

#include <cstdint>

namespace ControlReader {
// Inicializa GPIOs conforme HardwareMap
void init();

// Lê todos os controles e atualiza o buffer
void update();

// Acesso ao buffer de valores (thread-safe para ISR context)
const volatile uint8_t *getValues();
uint8_t getValue(uint8_t index);
uint8_t getNumControles();

// Pure processing functions (testable without hardware)
uint8_t mapAdcToMidi(uint16_t adcValue);
bool applyDeadzone(uint8_t newValue, uint8_t lastValue, uint8_t deadzone);
bool applyDebounce(bool reading, bool lastStable, uint32_t lastChangeMs,
                   uint32_t nowMs, uint16_t debounceMs);
uint8_t invertValue(uint8_t value);
uint8_t processEncoderTransition(uint8_t lastAB, uint8_t currentAB,
                                 uint8_t currentValue);
uint8_t processEncoderWithAccel(uint8_t lastAB, uint8_t currentAB,
                                uint8_t currentValue, uint32_t elapsedMs);
uint16_t applyEma(uint16_t currentFiltered, uint16_t newRaw, uint8_t alpha);
} // namespace ControlReader
