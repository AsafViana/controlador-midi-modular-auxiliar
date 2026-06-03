// Pure function implementations for native testing (no Arduino.h dependency)
// As funções puras agora vêm de PureFunctions.h (inline) via ControlReader.h.
// Este arquivo fornece apenas stubs de funções que dependem de hardware.

#include "../../src/hardware/ControlReader.h"
#include "../../src/hardware/HardwareMap.h"
#include "../../src/i2c/I2CSlave.h"
#include <cstring>

namespace ControlReader {

// Stubs para funções que dependem de hardware (GPIO)
// Não executam nada em testes nativos
void signalChange() {}
void clearSignal() {}

} // namespace ControlReader

namespace I2CSlave {

void serializeDescriptor(const ControleHW *controls, const uint8_t *values,
                         uint8_t count, uint8_t *outBuffer) {
  // First byte: number of controls
  outBuffer[0] = count;

  // For each control: tipo(1) + label(12, zero-padded) + valor(1) = 14 bytes
  for (uint8_t i = 0; i < count; i++) {
    uint8_t *entry = &outBuffer[1 + (i * 14)];

    // tipo: 1 byte (cast from TipoControle enum)
    entry[0] = static_cast<uint8_t>(controls[i].tipo);

    // label: 12 bytes, zero-padded
    memset(&entry[1], 0, 12);
    strncpy(reinterpret_cast<char *>(&entry[1]), controls[i].label, 12);

    // valor: 1 byte
    entry[13] = values[i];
  }
}

} // namespace I2CSlave
