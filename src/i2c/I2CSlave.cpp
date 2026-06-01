#include "I2CSlave.h"
#include "../config.h"
#include "../hardware/Calibration.h"
#include "../hardware/ControlReader.h"
#include "../hardware/HardwareMap.h"
#include "../hardware/PersistentConfig.h"
#include "../ota/OTAHandler.h"
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

// Último comando recebido (volatile: escrito em onReceive, lido em onRequest)
static volatile uint8_t lastCommand = 0;
static volatile bool activityFlag = false;

// Flag para restart após OTA (evita delay/restart dentro de ISR)
static volatile bool otaRestartPending = false;

// Spinlock para proteção de acesso ao valueBuffer entre loop e ISR
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// onReceive callback: store command if valid, ignore unknown commands
static void onReceive(int numBytes) {
  if (numBytes < 1)
    return;
  activityFlag = true;
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
  } else if (cmd == CMD_SET_CONFIG) {
    lastCommand = cmd;
    // Payload: [channel(1), deadzone(1), debounce_hi(1), debounce_lo(1),
    // invertido(1)]
    uint8_t payload[5];
    uint8_t idx = 0;
    while (Wire.available() && idx < 5) {
      payload[idx++] = Wire.read();
    }
    if (idx == 5) {
      PersistentConfig::applyFromI2C(payload, idx);
    }
  } else if (cmd == CMD_OTA_BEGIN) {
    lastCommand = cmd;
    if (Wire.available() >= 4) {
      uint32_t size = (uint32_t)Wire.read();
      size |= (uint32_t)Wire.read() << 8;
      size |= (uint32_t)Wire.read() << 16;
      size |= (uint32_t)Wire.read() << 24;
      OTAHandler::begin(size);
    }
  } else if (cmd == CMD_OTA_DATA) {
    lastCommand = cmd;
    if (Wire.available() >= 4) {
      uint32_t offset = (uint32_t)Wire.read();
      offset |= (uint32_t)Wire.read() << 8;
      offset |= (uint32_t)Wire.read() << 16;
      offset |= (uint32_t)Wire.read() << 24;
      uint8_t dataBuf[OTA_MAX_BLOCK_SIZE];
      uint16_t dataLen = 0;
      while (Wire.available() && dataLen < OTA_MAX_BLOCK_SIZE) {
        dataBuf[dataLen++] = Wire.read();
      }
      // Rejeita bloco se há dados excedentes (truncamento = corrupção)
      if (Wire.available()) {
        while (Wire.available()) {
          Wire.read();
        }
        // Bloco maior que OTA_MAX_BLOCK_SIZE — não processar
      } else if (dataLen > 0) {
        OTAHandler::writeBlock(offset, dataBuf, dataLen);
      }
    }
  } else if (cmd == CMD_OTA_END) {
    lastCommand = cmd;
    if (Wire.available() >= 4) {
      uint32_t crc = (uint32_t)Wire.read();
      crc |= (uint32_t)Wire.read() << 8;
      crc |= (uint32_t)Wire.read() << 16;
      crc |= (uint32_t)Wire.read() << 24;
      if (OTAHandler::end(crc)) {
        otaRestartPending = true;
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
  activityFlag = true;
  uint8_t cmd = lastCommand;

  if (cmd == CMD_DESCRIPTOR) {
    uint8_t buffer[1 + (14 * MAX_CONTROLES)];
    const volatile uint8_t *values = ControlReader::getValues();
    uint8_t count = HardwareMap::NUM_CONTROLES;

    // Cópia atômica dos valores (minimiza janela de inconsistência)
    uint8_t valsCopy[MAX_CONTROLES];
    portENTER_CRITICAL_SAFE(&spinlock);
    for (uint8_t i = 0; i < count; i++) {
      valsCopy[i] = values[i];
    }
    portEXIT_CRITICAL_SAFE(&spinlock);

    serializeDescriptor(HardwareMap::CONTROLES, valsCopy, count, buffer);
    uint16_t totalSize = 1 + (14 * count);
    Wire.write(buffer, totalSize);
  } else if (cmd == CMD_READ_VALUES) {
    const volatile uint8_t *values = ControlReader::getValues();
    uint8_t count = HardwareMap::NUM_CONTROLES;

    // Cópia atômica dos valores
    uint8_t valsCopy[MAX_CONTROLES];
    portENTER_CRITICAL_SAFE(&spinlock);
    for (uint8_t i = 0; i < count; i++) {
      valsCopy[i] = values[i];
    }
    portEXIT_CRITICAL_SAFE(&spinlock);

    for (uint8_t i = 0; i < count; i++) {
      Wire.write(valsCopy[i]);
    }

    // Master leu os valores — limpa sinalização de interrupt
    ControlReader::clearSignal();
  } else if (cmd == CMD_INFO) {
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

bool hasRecentActivity() {
  if (activityFlag) {
    activityFlag = false;
    return true;
  }
  return false;
}

bool isOtaRestartPending() { return otaRestartPending; }

// Tenta recuperar o barramento I2C se SDA estiver preso em LOW
static void recoverI2CBus() {
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
}

void init() {
  // Verifica se o barramento está travado ANTES de inicializar I2C
  pinMode(PIN_SDA, INPUT);
  pinMode(PIN_SCL, INPUT);
  if (digitalRead(PIN_SDA) == LOW) {
    recoverI2CBus();
  }

  Wire.setBufferSize(256);
  Wire.begin((uint8_t)I2C_ADDRESS, (int)PIN_SDA, (int)PIN_SCL);
  Wire.setTimeOut(I2C_TIMEOUT_MS);

  // Ativa pull-up interno como fallback (pull-ups externos 4.7-10kΩ ainda são
  // recomendados)
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);

  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
}

} // namespace I2CSlave
