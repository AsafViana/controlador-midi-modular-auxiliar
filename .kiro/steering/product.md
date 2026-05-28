# Product Context

## O que é este projeto

Firmware embarcado para ESP32-C3 que transforma o microcontrolador em um **módulo de extensão para controladores MIDI**. O módulo lê controles físicos (potenciômetros, botões, encoders rotativos, sensores analógicos) e disponibiliza os valores via barramento **I2C** para um módulo principal que gerencia a comunicação MIDI (USB/DIN).

## Arquitetura do sistema

```
┌─────────────────┐       I2C        ┌──────────────────┐
│  Módulo Principal│◄────────────────►│  Este Módulo     │
│  (MIDI USB/DIN)  │  SDA + SCL + GND │  (ESP32-C3)      │
└─────────────────┘                   └──────────────────┘
                                            │
                                      ┌─────┴─────┐
                                      │ Controles │
                                      │ Físicos   │
                                      └───────────┘
                                       Pots, Botões,
                                       Encoders, Sensores
```

## Público-alvo

- Makers e músicos que constroem controladores MIDI customizados
- Projeto pessoal/educacional com foco na comunidade maker/MIDI

## Características principais

- Até **16 controles** por módulo
- Até **8 módulos** no mesmo barramento I2C (endereços 0x20–0x27)
- Calibração automática de ADC por canal (NVS)
- Suavização EMA para leituras estáveis
- Detecção de pino flutuante (proteção contra cabo solto)
- Aceleração de encoder (giro rápido = saltos maiores)
- Suporte a encoder EC11 com push-button integrado
- Multiplexador analógico opcional (CD4051/CD4067)
- Modo de baixo consumo (light sleep após 30s inativo)
- Watchdog com auto-recovery
- Rate limiting para leituras analógicas (200 Hz)
- Atualização OTA via I2C
- Validação de GPIOs em tempo de compilação (static_assert)

## Hardware

| Componente | Especificação |
|---|---|
| MCU | ESP32-C3 (RISC-V, 160 MHz, 4 MB Flash, 400 KB SRAM) |
| ADC | 12-bit, 6 canais (GPIO 0-5) |
| GPIOs disponíveis | 0-10, 18-21 (~15 utilizáveis) |
| I2C | Slave, SDA=GPIO8, SCL=GPIO9 |
| Alimentação | 3.3V (via USB ou regulador) |

## Protocolo I2C

| Comando | Código | Descrição |
|---|---|---|
| CMD_DESCRIPTOR | 0x01 | Retorna mapa de controles |
| CMD_READ_VALUES | 0x02 | Retorna valores atuais |
| CMD_INFO | 0x03 | Versão, nome, chip ID |
| CMD_CALIBRATE | 0x04 | Inicia calibração ADC |
| CMD_SET_CONFIG | 0x05 | Configura parâmetros (NVS) |
| CMD_OTA_BEGIN | 0x10 | Inicia atualização de firmware |
| CMD_OTA_DATA | 0x11 | Bloco de dados OTA |
| CMD_OTA_END | 0x12 | Finaliza OTA com CRC32 |

## Integrações

- **Barramento I2C**: Comunicação com módulo principal (100 kHz, buffer 256 bytes)
- **NVS (Flash)**: Persistência de calibração e configurações
- **ESP-IDF APIs**: WDT, light sleep, GPIO wakeup
