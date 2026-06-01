<p align="center">
  <h1 align="center">🎛️ Módulo de Extensão MIDI</h1>
  <p align="center">
    Firmware modular para ESP32-C3 — expanda seu controlador MIDI com potenciômetros, botões, encoders e sensores via I2C.
  </p>
  <p align="center">
    <img src="https://img.shields.io/badge/plataforma-ESP32--C3-blue?style=flat-square" alt="ESP32-C3">
    <img src="https://img.shields.io/badge/framework-Arduino-teal?style=flat-square" alt="Arduino">
    <img src="https://img.shields.io/badge/build-PlatformIO-orange?style=flat-square" alt="PlatformIO">
    <img src="https://img.shields.io/badge/protocolo-I2C%20Slave-purple?style=flat-square" alt="I2C">
    <img src="https://img.shields.io/badge/versão-0.3.0-green?style=flat-square" alt="v0.3.0">
  </p>
</p>

---

## 📋 Visão Geral

Este firmware transforma um **ESP32-C3** em um módulo de extensão para controladores MIDI. Ele lê controles físicos (pots, botões, encoders, sensores) e disponibiliza os valores via barramento **I2C** para um módulo principal que gerencia a comunicação MIDI.

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

**Características:**

- Até **16 controles** por módulo
- Até **8 módulos** no mesmo barramento (endereços 0x20–0x27)
- Calibração automática de ADC por canal
- Suavização EMA para leituras estáveis
- Detecção de pino flutuante (proteção contra cabo solto)
- Aceleração de encoder (giro rápido = saltos maiores)
- Modo de baixo consumo (light sleep após 30s inativo)
- Watchdog com auto-recovery
- Atualização OTA via I2C

---

## ⚡ Início Rápido

```bash
# 1. Clone o repositório
git clone https://github.com/seu-usuario/controlador-midi-modular-auxiliar.git

# 2. Abra no VS Code com PlatformIO instalado

# 3. Edite src/hardware/HardwareMap.h com seus controles

# 4. Conecte o ESP32-C3 via USB e faça upload
pio run -t upload
```

---

## 🔌 Mapa de GPIOs

### Pinos disponíveis para controles

| GPIO | Analógico (ADC) | Digital | Observação |
|:----:|:---------------:|:-------:|:-----------|
| 0    | ✅ | ✅ | ADC + Digital |
| 1    | ✅ | ✅ | ADC + Digital |
| 2    | ✅ | ✅ | ADC + Digital |
| 3    | ✅ | ✅ | ADC + Digital |
| 4    | ✅ | ✅ | ADC + Digital |
| 5    | ✅ | ✅ | ADC + Digital |
| 6    | ❌ | ✅ | Digital apenas |
| 7    | ❌ | ✅ | Digital apenas |
| 18   | ❌ | ✅ | Digital apenas |
| 19   | ❌ | ✅ | Digital apenas |
| 20   | ❌ | ✅ | Digital apenas |
| 21   | ❌ | ✅ | Digital apenas |

### Pinos reservados — NÃO USE

| GPIO | Função |
|:----:|:-------|
| 8    | 🔒 I2C SDA (comunicação com módulo principal) |
| 9    | 🔒 I2C SCL (comunicação com módulo principal) |
| 10   | 🔒 INT (sinal de interrupt para o módulo principal — wire-OR open-drain) |

---

## 🎚️ Conexões de Hardware

### Potenciômetro (10kΩ linear)

```
    3.3V ────────┐
                 │
            ┌────┴────┐
            │   POT   │
            │  10kΩ B │
            └────┬────┘
                 │
    GPIO 0~5 ───┤ (cursor / pino do meio)
                 │
            ┌────┴────┐
            │   POT   │
            └────┬────┘
                 │
    GND ─────────┘
```

| Pino do pot | Conectar em |
|:-----------:|:------------|
| Esquerdo    | GND |
| Centro (cursor) | GPIO analógico (0–5) |
| Direito     | 3.3V |

---

### Botão (push-button)

```
    GPIO (0~10, 18~21) ──── ┤ BTN ├ ──── GND
                            
    (pull-up interno ativado pelo firmware)
```

| Pino do botão | Conectar em |
|:-------------:|:------------|
| Terminal 1    | GPIO digital |
| Terminal 2    | GND |

> Não precisa de resistor externo.

---

### Sensor analógico (FSR, LDR, flex)

```
    3.3V ──── SENSOR ────┬──── GPIO (0~5)
                         │
                        [R]  10kΩ
                         │
    GND ─────────────────┘
```

| Conexão | Onde |
|:-------:|:-----|
| Sensor terminal 1 | 3.3V |
| Sensor terminal 2 | GPIO analógico + resistor 10kΩ para GND |

---

### Encoder rotativo (EC11)

```
    GPIO A ─────── Pino A
    GPIO B ─────── Pino B
    GND ────────── Pino C (comum)
    
    Se tiver push-button:
    GPIO SW ────── Pino SW
    GND ────────── Outro terminal SW
```

| Pino do encoder | Conectar em |
|:---------------:|:------------|
| A | GPIO digital |
| B | GPIO digital (diferente de A) |
| C (comum) | GND |
| SW (push) | GPIO digital (opcional) |
| Outro terminal SW | GND |

> Pull-ups internos ativados automaticamente.

---

### Barramento I2C (conexão com módulo principal)

```
    ESP32-C3                Módulo Principal
    ┌──────────┐            ┌──────────────┐
    │ GPIO 8   ├────SDA─────┤ SDA          │
    │ GPIO 9   ├────SCL─────┤ SCL          │
    │ GND      ├────GND─────┤ GND          │
    │ 3.3V     ├────VCC─────┤ 3.3V         │
    └──────────┘            └──────────────┘
```

> **Cabo > 30cm?** Adicione resistores de **4.7kΩ** entre SDA↔3.3V e SCL↔3.3V.

### Sinal de Interrupt (GPIO 10 → Módulo Principal)

O módulo auxiliar sinaliza **ativamente** quando um controle muda, sem esperar polling do mestre.

```
    ESP32-C3                Módulo Principal
    ┌──────────┐            ┌──────────────┐
    │ GPIO 8   ├────SDA─────┤ SDA          │
    │ GPIO 9   ├────SCL─────┤ SCL          │
    │ GPIO 10  ├────INT─────┤ GPIO 6 (INT) │  ← NOVO
    │ GND      ├────GND─────┤ GND          │
    │ 3.3V     ├────VCC─────┤ 3.3V         │
    └──────────┘            └──────────────┘
```

**Como funciona:**

1. Controle muda (pot/botão/encoder) → GPIO 10 vai para **LOW**
2. Módulo principal detecta a borda de descida → lê valores via I2C
3. Após leitura, GPIO 10 volta para **HIGH** (idle)

**Wire-OR (múltiplos módulos):**

O pino INT usa saída **open-drain** — múltiplos módulos podem compartilhar o mesmo fio:

```
    Módulo #1 GPIO 10 ──┐
    Módulo #2 GPIO 10 ──┼──── Principal GPIO 6 (INPUT_PULLUP)
    Módulo #3 GPIO 10 ──┘
```

Qualquer módulo que tiver dados novos puxa a linha para LOW. O pull-up do mestre mantém HIGH quando nenhum módulo está sinalizando.

> **Cabo > 30cm?** Adicione pull-up externo de **4.7–10kΩ** na linha INT do lado do mestre.

---

## ⚙️ Configuração do Firmware

### 1. Definir seus controles — `src/hardware/HardwareMap.h`

Este é o **único arquivo que você precisa editar**. Cada linha na lista `CONTROLES[]` representa um controle físico:

```cpp
constexpr ControleHW CONTROLES[] = {
    // {"Nome",   GPIO, Tipo,                       CC, Inv, GPIO_B, GPIO_SW}
    {"Volume",    0,    TipoControle::POTENCIOMETRO, 7,  false, 0, 0},
    {"Filtro",    1,    TipoControle::POTENCIOMETRO, 74, false, 0, 0},
    {"Play",      6,    TipoControle::BOTAO,         20, false, 0, 0},
    {"Encoder1",  18,   TipoControle::ENCODER,       10, false, 19, 20},
};
```

#### Referência dos campos

| # | Campo | Descrição | Valores |
|:-:|:------|:----------|:--------|
| 1 | **Nome** | Identificador do controle | Texto até 12 caracteres |
| 2 | **GPIO** | Pino principal | `0–5` (pot/sensor) · `0–10, 18–21` (botão/encoder) |
| 3 | **Tipo** | Tipo de controle | `POTENCIOMETRO` · `BOTAO` · `SENSOR` · `ENCODER` |
| 4 | **CC** | Número MIDI Control Change | `1–127` (único por controle) |
| 5 | **Invertido** | Inverte direção do valor | `false` · `true` |
| 6 | **GPIO_B** | Pino B do encoder | Número do GPIO ou `0` |
| 7 | **GPIO_SW** | Push-button do encoder | Número do GPIO ou `0` |

---

### 2. Endereço I2C — `src/config.h`

```cpp
constexpr uint8_t I2C_ADDRESS = 0x20;  // Altere para 0x21, 0x22... se usar múltiplos módulos
```

### 3. Nome do módulo — `src/config.h`

```cpp
constexpr const char *MODULE_NAME = "EXT-01";  // Ex: "FADERS", "DRUMS", "KNOBS"
```

---

## 📐 Exemplos de Configuração

<details>
<summary><b>🎚️ Módulo de 4 faders + 2 botões</b></summary>

```cpp
constexpr ControleHW CONTROLES[] = {
    {"Volume",  0, TipoControle::POTENCIOMETRO, 7,  false, 0, 0},
    {"Pan",     1, TipoControle::POTENCIOMETRO, 10, false, 0, 0},
    {"Reverb",  2, TipoControle::POTENCIOMETRO, 91, false, 0, 0},
    {"Delay",   3, TipoControle::POTENCIOMETRO, 92, false, 0, 0},
    {"Mute",    6, TipoControle::BOTAO,         20, false, 0, 0},
    {"Solo",    7, TipoControle::BOTAO,         21, false, 0, 0},
};
```

</details>

<details>
<summary><b>🥁 Módulo de 8 pads (botões)</b></summary>

```cpp
constexpr ControleHW CONTROLES[] = {
    {"Pad1", 0,  TipoControle::BOTAO, 36, false, 0, 0},
    {"Pad2", 1,  TipoControle::BOTAO, 37, false, 0, 0},
    {"Pad3", 2,  TipoControle::BOTAO, 38, false, 0, 0},
    {"Pad4", 3,  TipoControle::BOTAO, 39, false, 0, 0},
    {"Pad5", 4,  TipoControle::BOTAO, 40, false, 0, 0},
    {"Pad6", 5,  TipoControle::BOTAO, 41, false, 0, 0},
    {"Pad7", 6,  TipoControle::BOTAO, 42, false, 0, 0},
    {"Pad8", 7,  TipoControle::BOTAO, 43, false, 0, 0},
};
```

</details>

<details>
<summary><b>🔄 Módulo de 2 encoders + 1 pot</b></summary>

```cpp
constexpr ControleHW CONTROLES[] = {
    {"Cutoff",  0,  TipoControle::POTENCIOMETRO, 74, false, 0,  0},
    {"EncVol",  18, TipoControle::ENCODER,       7,  false, 19, 20},
    {"EncPan",  6,  TipoControle::ENCODER,       10, false, 7,  10},
};
```

</details>

<details>
<summary><b>🎵 Módulo misto completo (6 pots + 4 botões + 2 encoders)</b></summary>

```cpp
constexpr ControleHW CONTROLES[] = {
    // Potenciômetros (GPIO 0-5)
    {"Vol",     0, TipoControle::POTENCIOMETRO, 7,  false, 0, 0},
    {"Pan",     1, TipoControle::POTENCIOMETRO, 10, false, 0, 0},
    {"Bass",    2, TipoControle::POTENCIOMETRO, 71, false, 0, 0},
    {"Mid",     3, TipoControle::POTENCIOMETRO, 72, false, 0, 0},
    {"Treble",  4, TipoControle::POTENCIOMETRO, 73, false, 0, 0},
    {"Reverb",  5, TipoControle::POTENCIOMETRO, 91, false, 0, 0},
    // Botões (GPIO 6, 7, 10, 21)
    {"Mute",    6,  TipoControle::BOTAO, 20, false, 0, 0},
    {"Solo",    7,  TipoControle::BOTAO, 21, false, 0, 0},
    {"Rec",     10, TipoControle::BOTAO, 22, false, 0, 0},
    {"Play",    21, TipoControle::BOTAO, 23, false, 0, 0},
    // Encoders (GPIO 18/19 e 20 como switch do primeiro)
    {"Scroll",  18, TipoControle::ENCODER, 74, false, 19, 20},
};
```

</details>

---

## 🔀 Multiplexador Analógico (opcional)

Para mais de 6 potenciômetros, use um **CD4051** (8ch) ou **CD4067** (16ch).

### Conexão do CD4051

| Pino CD4051 | Conectar em |
|:-----------:|:------------|
| SIG (3)     | GPIO ADC (0–5) |
| S0 (11)     | GPIO digital |
| S1 (10)     | GPIO digital |
| S2 (9)      | GPIO digital |
| VCC (16)    | 3.3V |
| GND (8)     | GND |
| INH (6)     | GND |
| VEE (7)     | GND |
| Y0–Y7      | Cursores dos potenciômetros |

### Configuração — `src/hardware/AnalogMux.h`

```cpp
constexpr MuxConfig MUX_CONFIG[] = {
    // {GPIO_sinal, S0, S1, S2, S3, canais}
    {5, 6, 7, 10, 0, 8},   // CD4051: sinal GPIO 5, seleção 6/7/10
};
```

---

## 📦 Gravando o Firmware

### Pré-requisitos

- [VS Code](https://code.visualstudio.com/) com extensão [PlatformIO](https://platformio.org/install/ide?install=vscode)
- Cabo USB conectado ao ESP32-C3

### Upload

```bash
# Via CLI
pio run -t upload

# Ou use o botão ➡️ (Upload) na barra inferior do VS Code
```

### Validação em tempo de compilação

O firmware **não compila** se a configuração de GPIOs estiver incorreta:

```
error: static assertion failed: Invalid GPIO configuration detected!
       ADC controls (POT/SENSOR) must use GPIO 0-5.
       Digital controls (BUTTON/ENCODER) must use GPIO 0-10 or 18-21.
```

Isso é uma proteção — corrija o `HardwareMap.h` e tente novamente.

---

## 📊 Limites do Sistema

| Parâmetro | Valor |
|:----------|:------|
| Controles por módulo | 16 máx. |
| Entradas analógicas (sem mux) | 6 |
| Entradas analógicas (com CD4051) | +8 por mux |
| Entradas analógicas (com CD4067) | +16 por mux |
| Módulos no barramento I2C | 8 (0x20–0x27) |
| Resolução ADC | 12 bits → 7 bits MIDI |
| Taxa de leitura analógica | 200 Hz |
| Cabo I2C sem pull-up externo | ≤ 30 cm |
| Cabo I2C com pull-up 4.7kΩ | ≤ 1–2 m |

---

## 🛠️ Dicas de Montagem

| # | Dica |
|:-:|:-----|
| 1 | **Teste um controle por vez** — conecte, grave, verifique, depois adicione os próximos |
| 2 | **Capacitor 100nF** entre 3.3V e GND próximo ao ESP32 reduz ruído no ADC |
| 3 | **Fios curtos** nos encoders (< 15cm) evitam leituras erradas |
| 4 | **Pot não chega a 0 ou 127?** A calibração automática resolve isso |
| 5 | **Cabo I2C longo?** Pull-ups de 4.7kΩ são obrigatórios acima de 30cm |
| 6 | **Pot invertido?** Use `true` no campo Invertido ao invés de trocar fios |

---

## 🔍 Resolução de Problemas

| Sintoma | Causa provável | Solução |
|:--------|:---------------|:--------|
| Não compila | GPIO inválido | Pots → GPIO 0–5 · Botões/Encoders → GPIO 0–10, 18–21 |
| Pot não atinge extremos | ADC não-linear | Calibração via módulo principal |
| Valor oscila parado | Ruído elétrico | Capacitor 100nF entre cursor e GND |
| Botão dispara sozinho | Mau contato / sem GND | Verificar soldas e conexão GND |
| Encoder pula valores | Interferência | Fios curtos + capacitor 100nF em A e B |
| Módulo não responde I2C | Endereço duplicado | Cada módulo precisa de endereço único |
| Módulo reinicia sozinho | Watchdog (normal) | Auto-recovery — não é defeito |

---

## 📁 Estrutura do Projeto

```
├── src/
│   ├── config.h                 ← Configurações gerais (endereço I2C, nome, limites)
│   ├── main.cpp                 ← Loop principal
│   ├── hardware/
│   │   ├── HardwareMap.h        ← ⭐ EDITE AQUI: definição dos seus controles
│   │   ├── AnalogMux.h          ← Configuração de multiplexadores (opcional)
│   │   ├── ControlReader.cpp/h  ← Leitura e processamento dos controles
│   │   ├── Calibration.cpp/h    ← Calibração ADC por canal
│   │   └── PersistentConfig.h   ← Configurações salvas na flash (NVS)
│   ├── i2c/
│   │   └── I2CSlave.cpp/h       ← Comunicação I2C com módulo principal
│   └── ota/
│       └── OTAHandler.cpp/h     ← Atualização de firmware via I2C
├── docs/
│   └── melhorias-produto.md     ← Roadmap de melhorias
├── platformio.ini               ← Configuração de build
└── README.md                    ← Este arquivo
```

---

## 📄 Licença

Este projeto é de uso pessoal/educacional. Consulte o autor para uso comercial.

---

<p align="center">
  <sub>Feito com ❤️ para a comunidade maker/MIDI</sub>
</p>
