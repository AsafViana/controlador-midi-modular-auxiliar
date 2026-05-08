#pragma once

#include <cstdint>

// Versão do firmware
constexpr uint8_t FW_VERSION_MAJOR = 0;
constexpr uint8_t FW_VERSION_MINOR = 1;
constexpr uint8_t FW_VERSION_PATCH = 0;

// Endereço I2C do módulo (faixa válida: 0x20-0x27)
constexpr uint8_t I2C_ADDRESS = 0x20;

// Nome do módulo
constexpr const char *MODULE_NAME = "EXT-01";

// Pinos I2C
constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;

// Parâmetros de leitura
constexpr uint8_t DEADZONE =
    2; // Tolerância para leituras analógicas (em unidades MIDI 0-127)
constexpr uint16_t DEBOUNCE_MS = 50; // Tempo de debounce para botões (ms)

// Limites do protocolo
constexpr uint8_t MAX_CONTROLES = 16;

// Watchdog Timer
constexpr uint16_t WDT_TIMEOUT_MS = 5000; // 5 segundos

// Timeout I2C (ms)
constexpr uint16_t I2C_TIMEOUT_MS = 1000;

// Comandos I2C
constexpr uint8_t CMD_DESCRIPTOR = 0x01;
constexpr uint8_t CMD_READ_VALUES = 0x02;

// Constantes MIDI
constexpr uint8_t MIDI_MAX = 127;
constexpr uint8_t MIDI_MID = 64;

// ADC
constexpr uint16_t ADC_MAX = 4095; // ESP32-C3: 12-bit ADC
