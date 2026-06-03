#pragma once

#include "PureFunctions.h"
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

// Sinalização de mudança para o Master via GPIO interrupt
void signalChange(); // Puxa PIN_INT_OUT para LOW (dados novos)
void clearSignal();  // Restaura PIN_INT_OUT para HIGH (idle)
} // namespace ControlReader
