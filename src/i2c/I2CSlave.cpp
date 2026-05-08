#include "I2CSlave.h"
#include "../config.h"
#include "../hardware/Calibration.h"
#include "../hardware/ControlReader.h"
#include "../hardware/HardwareMap.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstring>

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

// Static variable to store the last valid command received
static uint8_t lastCommand = 0;

// onReceive callback: store command if valid, ignore unknown commands
static void onReceive(int numBytes) {
  if (numBytes < 1)
    return;
  uint8_t cmd = Wire.read();
  if (cmd == CMD_DESCRIPTOR || cmd == CMD_READ_VALUES || cmd == CMD_INFO) {
    lastCommand = cmd;
  } else if (cmd == CMD_CALIBRATE) {
    lastCommand = cmd;
    // Segundo byte: canal a calibrar (0xFF = parar calibração)
    if (Wire.available()) {
      uint8_t channel = Wire.read();
      if (channel == 0xFF) {
        Calibration::stopCalibration();
      } else {
        Calibration::startCalibration(channel);
      }
    }
  }
  // Discard any remaining bytes
  while (Wire.available()) {
    Wire.read();
  }
}

// onRequest callback: respond based on last valid command
static void onRequest() {
  if (lastCommand == CMD_DESCRIPTOR) {
    uint8_t buffer[1 + (14 * MAX_CONTROLES)];
    const volatile uint8_t *values = ControlReader::getValues();
    uint8_t count = HardwareMap::NUM_CONTROLES;

    // Cast volatile values to non-volatile for serialization
    uint8_t valsCopy[MAX_CONTROLES];
    for (uint8_t i = 0; i < count; i++) {
      valsCopy[i] = values[i];
    }

    serializeDescriptor(HardwareMap::CONTROLES, valsCopy, count, buffer);
    uint16_t totalSize = 1 + (14 * count);
    Wire.write(buffer, totalSize);
  } else if (lastCommand == CMD_READ_VALUES) {
    const volatile uint8_t *values = ControlReader::getValues();
    uint8_t count = HardwareMap::NUM_CONTROLES;
    for (uint8_t i = 0; i < count; i++) {
      Wire.write((uint8_t)values[i]);
    }
  } else if (lastCommand == CMD_INFO) {
    // Formato: version(3) + name(12, zero-padded) + chipId(4) = 19 bytes
    uint8_t infoBuffer[19];
    memset(infoBuffer, 0, sizeof(infoBuffer));

    // Versão: major.minor.patch
    infoBuffer[0] = FW_VERSION_MAJOR;
    infoBuffer[1] = FW_VERSION_MINOR;
    infoBuffer[2] = FW_VERSION_PATCH;

    // Nome do módulo (12 bytes, zero-padded)
    strncpy(reinterpret_cast<char *>(&infoBuffer[3]), MODULE_NAME, 12);

    // Chip ID único (últimos 4 bytes do MAC address)
    uint64_t mac = ESP.getEfuseMac();
    infoBuffer[15] = (uint8_t)(mac);
    infoBuffer[16] = (uint8_t)(mac >> 8);
    infoBuffer[17] = (uint8_t)(mac >> 16);
    infoBuffer[18] = (uint8_t)(mac >> 24);

    Wire.write(infoBuffer, sizeof(infoBuffer));
  }
}

// Tenta recuperar o barramento I2C se SDA estiver preso em LOW
static void recoverI2CBus() {
  Wire.end();
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, OUTPUT);

  // Toggle SCL até 9 vezes para liberar SDA
  for (uint8_t i = 0; i < 9; i++) {
    digitalWrite(PIN_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(5);
    if (digitalRead(PIN_SDA) == HIGH) {
      break; // SDA liberado
    }
  }

  // Reinicializa o barramento
  Wire.setBufferSize(256);
  Wire.begin((uint8_t)I2C_ADDRESS, (int)PIN_SDA, (int)PIN_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

void init() {
  Wire.setBufferSize(256);
  Wire.begin((uint8_t)I2C_ADDRESS, (int)PIN_SDA, (int)PIN_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);

  // Verifica se o barramento está travado na inicialização
  pinMode(PIN_SDA, INPUT);
  if (digitalRead(PIN_SDA) == LOW) {
    recoverI2CBus();
  }
}

} // namespace I2CSlave
