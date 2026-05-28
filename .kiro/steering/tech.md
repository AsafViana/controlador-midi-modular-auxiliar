# Technical Standards — Firmware ESP32-C3

## Stack

- **MCU**: ESP32-C3 (RISC-V single-core, 160 MHz)
- **Framework**: Arduino (via PlatformIO)
- **Linguagem**: C++17
- **Build system**: PlatformIO
- **Testes**: RapidCheck (property-based, env:native)
- **IDE**: VS Code + PlatformIO Extension

## Arquitetura do firmware

```
src/
├── config.h                 ← Configurações globais (constexpr)
├── main.cpp                 ← setup() + loop()
├── hardware/
│   ├── HardwareMap.h        ← Definição dos controles (editável pelo usuário)
│   ├── AnalogMux.h/.cpp     ← Multiplexador analógico (CD4051/CD4067)
│   ├── ControlReader.h/.cpp ← Leitura e processamento dos controles
│   ├── Calibration.h/.cpp   ← Calibração ADC por canal (NVS)
│   └── PersistentConfig.h/.cpp ← Configurações salvas na flash
├── i2c/
│   └── I2CSlave.h/.cpp      ← Comunicação I2C com módulo principal
└── ota/
    └── OTAHandler.h/.cpp    ← Atualização de firmware via I2C
```

### Regras de organização

- `config.h` contém TODAS as constantes globais como `constexpr`
- `HardwareMap.h` é o ÚNICO arquivo que o usuário final precisa editar
- Cada módulo (hardware/, i2c/, ota/) é independente e testável isoladamente
- Headers (.h) contêm declarações; implementação nos .cpp correspondentes
- Sem dependências circulares entre módulos

## Convenções de código

### Nomenclatura

| Elemento | Convenção | Exemplo |
|---|---|---|
| Classes/Structs | PascalCase | `ControlReader`, `ControleHW` |
| Métodos/Funções | camelCase ou PascalCase (estilo Arduino) | `init()`, `update()` |
| Constantes | UPPER_SNAKE_CASE ou camelCase com `constexpr` | `MAX_CONTROLES`, `EMA_ALPHA` |
| Variáveis locais | camelCase | `lastValue`, `rawAdc` |
| Variáveis estáticas | prefixo `s_` ou sem prefixo com `static` | `static uint32_t lastActivityMs` |
| Enums | PascalCase (tipo) + UPPER_SNAKE_CASE (valores) | `TipoControle::POTENCIOMETRO` |
| Namespaces | PascalCase | `ControlReader`, `I2CSlave` |
| Macros | UPPER_SNAKE_CASE (evitar — preferir constexpr) | — |
| Arquivos | PascalCase para classes, camelCase para utilitários | `ControlReader.h` |

### Estilo de código

- Indentação: 2 espaços
- Chaves: estilo Allman ou K&R (manter consistência no arquivo)
- Limite de linha: 100 caracteres (soft), 120 (hard)
- Um header guard com `#pragma once`
- Includes ordenados: Arduino/ESP-IDF → bibliotecas → headers do projeto

### Constantes e configuração

- SEMPRE usar `constexpr` ao invés de `#define` para constantes
- Toda constante configurável fica em `config.h`
- Validações em tempo de compilação com `static_assert`
- Tipos de tamanho explícito: `uint8_t`, `uint16_t`, `uint32_t` (não usar `int` genérico)

### Memória e performance

- Evitar alocação dinâmica (`new`, `malloc`) — tudo estático ou na stack
- Arrays com tamanho fixo definido por constexpr
- Usar `constexpr` arrays para dados de configuração em flash
- Preferir ponto fixo sobre ponto flutuante (ESP32-C3 não tem FPU)
- Rate limiting explícito para leituras ADC (evitar polling excessivo)

## Testes

### Framework: RapidCheck (property-based testing)

```cpp
// test/test_native_example/test_main.cpp
#include <rapidcheck.h>

int main() {
    rc::check("propriedade do mapeamento ADC",
        [](uint16_t raw) {
            RC_PRE(raw <= 4095);
            auto midi = mapAdcToMidi(raw);
            RC_ASSERT(midi <= 127);
        });
    return 0;
}
```

### Convenções de teste

- Testes nativos em `test/test_native*/`
- Nomes descritivos da propriedade testada
- `RC_PRE` para pré-condições, `RC_ASSERT` para asserções
- Testar edge cases: 0, máximo, limites de overflow
- Mock de hardware via interfaces (para testes nativos)

### Ambiente de teste

- `env:native` no platformio.ini (compila para desktop)
- Bibliotecas de hardware mockadas ou abstraídas
- Build com `-std=c++17`

## GPIO — Regras de hardware

### Pinos disponíveis

| GPIO | ADC | Digital | Observação |
|------|-----|---------|------------|
| 0-5 | ✅ | ✅ | Potenciômetros e sensores |
| 6-7, 10 | ❌ | ✅ | Botões e encoders apenas |
| 18-21 | ❌ | ✅ | Botões e encoders apenas |
| 8, 9 | ❌ | ❌ | **RESERVADOS** (I2C SDA/SCL) |

### Validação em tempo de compilação

```cpp
static_assert(gpio >= 0 && gpio <= 5, "Potenciômetros devem usar GPIO 0-5");
static_assert(gpio != 8 && gpio != 9, "GPIO 8 e 9 são reservados para I2C");
```

## I2C — Regras de comunicação

- Modo: Slave (endereço configurável 0x20-0x27)
- Clock: Standard mode (100 kHz)
- Buffer: 256 bytes (`Wire.setBufferSize`)
- Timeout: 1000ms (`Wire.setTimeOut`)
- Recovery: Toggle SCL se SDA stuck em LOW
- Protocolo: comando (1 byte) → resposta (N bytes)

## Segurança e robustez

- Watchdog Timer (5s) com auto-reset
- Detecção de pino flutuante — congela último valor válido
- Light sleep após 30s sem atividade I2C
- Validação de GPIOs em compile-time (static_assert)
- Sem undefined behavior: todos os GPIO verificados antes de uso
- Debounce por software para botões (50ms padrão)
- EMA para suavização de ADC (alpha configurável)
