#pragma once

#include <cstdint>

// Versão do firmware
constexpr uint8_t FW_VERSION_MAJOR = 0;
constexpr uint8_t FW_VERSION_MINOR = 3;
constexpr uint8_t FW_VERSION_PATCH = 0;

// Endereço I2C do módulo (faixa válida: 0x20-0x27)
constexpr uint8_t I2C_ADDRESS = 0x20;

// Nome do módulo
constexpr const char *MODULE_NAME = "EXT-01";

// Pinos I2C
constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;

// Pino de interrupção para sinalizar mudanças ao Master (open-drain)
// LOW = há dados novos; HIGH-Z = idle (pull-up externo no Master)
// Open-drain permite wire-OR: múltiplos módulos no mesmo fio
constexpr uint8_t PIN_INT_OUT = 10;

// Parâmetros de leitura
constexpr uint8_t DEADZONE =
    2; // Tolerância para leituras analógicas (em unidades MIDI 0-127)
constexpr uint16_t DEBOUNCE_MS = 50; // Tempo de debounce para botões (ms)

// Suavização (EMA - Exponential Moving Average)
// Alpha em ponto fixo: alpha = EMA_ALPHA / EMA_SCALE
// Valor padrão: 0.2 (20% novo, 80% anterior) — bom equilíbrio entre resposta e
// estabilidade
constexpr uint8_t EMA_ALPHA = 51; // ~0.2 * 256
constexpr uint16_t EMA_SCALE = 256;

// Detecção de pino flutuante
constexpr uint8_t FLOAT_DETECT_THRESHOLD =
    30; // Variação MIDI máxima em janela curta
constexpr uint16_t FLOAT_DETECT_WINDOW_MS = 50; // Janela de tempo para detecção

// Aceleração de encoder
constexpr uint16_t ENC_ACCEL_FAST_MS =
    50; // Intervalo entre pulsos para velocidade rápida
constexpr uint16_t ENC_ACCEL_MED_MS = 150; // Intervalo para velocidade média
constexpr uint8_t ENC_ACCEL_FAST_STEP = 8; // Incremento em velocidade rápida
constexpr uint8_t ENC_ACCEL_MED_STEP = 4;  // Incremento em velocidade média

// Limites do protocolo
constexpr uint8_t MAX_CONTROLES = 16;

// Watchdog Timer
constexpr uint32_t WDT_TIMEOUT_MS = 5000; // 5 segundos

// Timeout I2C (ms)
constexpr uint16_t I2C_TIMEOUT_MS = 1000;

// Modo de baixo consumo
constexpr uint32_t SLEEP_TIMEOUT_MS = 30000; // 30s sem requisição → light sleep

// Rate limiting para leitura analógica
constexpr uint8_t ANALOG_READ_INTERVAL_MS =
    5; // Ler analógicos a cada 5ms (200Hz)

// Comandos I2C
constexpr uint8_t CMD_DESCRIPTOR = 0x01;
constexpr uint8_t CMD_READ_VALUES = 0x02;
constexpr uint8_t CMD_INFO = 0x03;
constexpr uint8_t CMD_CALIBRATE = 0x04;
constexpr uint8_t CMD_SET_CONFIG = 0x05;
constexpr uint8_t CMD_OTA_BEGIN = 0x10; // Inicia OTA: payload = [size(4)]
constexpr uint8_t CMD_OTA_DATA =
    0x11; // Bloco de dados: payload = [offset(4) + data(N)]
constexpr uint8_t CMD_OTA_END = 0x12; // Finaliza OTA: payload = [crc32(4)]

// Constantes MIDI
constexpr uint8_t MIDI_MAX = 127;
constexpr uint8_t MIDI_MID = 64;

// ADC
constexpr uint16_t ADC_MAX = 4095; // ESP32-C3: 12-bit ADC

// OTA
constexpr uint16_t OTA_MAX_BLOCK_SIZE =
    240; // Máximo de bytes por bloco OTA via I2C
