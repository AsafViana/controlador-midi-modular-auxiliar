# Guia de Montagem — Módulo de Extensão MIDI

Este guia é para quem vai montar o hardware do módulo de extensão, baixar o firmware e configurar os pinos para o seu controlador. Não é necessário conhecimento de programação — apenas editar um arquivo de configuração simples.

---

## O que você vai precisar

### Placa

- **ESP32-C3 DevKitC-02** (ou módulo ESP32-C3-MINI-1 em PCB custom)
- Alimentação: 3.3V via USB ou regulador externo

### Controles suportados

| Controle | Exemplo de componente |
|---|---|
| Potenciômetro | Pot linear 10kΩ (B10K) |
| Botão | Push-button normalmente aberto |
| Sensor analógico | FSR (força), LDR (luz), etc. |
| Encoder rotativo | EC11 com ou sem push-button |

### Conexão com o módulo principal

- Cabo I2C de 4 fios (GND, 3.3V, SDA, SCL)
- Conector recomendado: JST-SH 4 pinos (padrão Qwiic/STEMMA QT)

---

## Mapa de GPIOs do ESP32-C3

Nem todos os pinos servem para tudo. Aqui está o resumo:

### Pinos para potenciômetros e sensores (analógicos)

| GPIO | Função |
|------|--------|
| 0 | ADC — pode conectar pot ou sensor |
| 1 | ADC — pode conectar pot ou sensor |
| 2 | ADC — pode conectar pot ou sensor |
| 3 | ADC — pode conectar pot ou sensor |
| 4 | ADC — pode conectar pot ou sensor |
| 5 | ADC — pode conectar pot ou sensor |

**Total: 6 entradas analógicas disponíveis.**

### Pinos para botões e encoders (digitais)

| GPIO | Função |
|------|--------|
| 0–5 | Digital (também podem ser usados como ADC) |
| 6 | Digital apenas |
| 7 | Digital apenas |
| 18 | Digital apenas |
| 19 | Digital apenas |
| 20 | Digital apenas |
| 21 | Digital apenas |

### Pinos reservados (NÃO USE para controles)

| GPIO | Motivo |
|------|--------|
| 8 | Reservado para I2C (SDA) |
| 9 | Reservado para I2C (SCL) |
| 10 | Reservado para INT (sinal de interrupt para o módulo principal) |

---

## Como conectar cada tipo de controle

### Potenciômetro (10kΩ linear)

```
        ESP32-C3
           │
    3.3V ──┤
           │
    GPIO ──┤◄── Pino do meio (cursor) do pot
           │
     GND ──┤
```

Conexão dos 3 pinos do potenciômetro:

1. **Pino esquerdo** → GND
2. **Pino do meio (cursor)** → GPIO analógico (0 a 5)
3. **Pino direito** → 3.3V

> Se o pot funcionar "ao contrário" (girar para direita diminui o valor), você pode inverter na configuração sem trocar fios.

---

### Botão (push-button)

```
    GPIO ──┬── Botão ── GND
           │
    (pull-up interno ativado automaticamente)
```

Conexão:

1. **Um terminal do botão** → GPIO digital
2. **Outro terminal** → GND

Não precisa de resistor externo — o firmware ativa o pull-up interno automaticamente.

---

### Sensor analógico (FSR, LDR, etc.)

```
    3.3V ── Sensor ──┬── GPIO (ADC)
                     │
                     R (10kΩ) ── GND
```

Conexão (divisor resistivo):

1. **Um terminal do sensor** → 3.3V
2. **Outro terminal** → GPIO analógico (0 a 5) + resistor de 10kΩ para GND

O resistor de 10kΩ forma um divisor de tensão com o sensor. Sem ele, a leitura não funciona corretamente.

---

### Encoder rotativo (EC11 ou similar)

```
    GPIO A ── Pino A do encoder
    GPIO B ── Pino B do encoder
    GND    ── Pino C (comum) do encoder
```

Conexão:

1. **Pino A** → GPIO digital
2. **Pino B** → GPIO digital (diferente do A)
3. **Pino C (comum/central)** → GND

Se o encoder tiver **push-button integrado**:
4. **Pino SW** → GPIO digital (um terceiro pino)
5. **Outro terminal do switch** → GND

Pull-ups internos são ativados automaticamente nos 3 pinos.

---

### Conexão I2C com o módulo principal

```
    ESP32-C3          Módulo Principal
    ────────          ────────────────
    GPIO 8 (SDA) ──── SDA
    GPIO 9 (SCL) ──── SCL
    GND          ──── GND
    3.3V         ──── 3.3V (se alimentado pelo mestre)
```

**Importante:**

- Se o cabo I2C tiver mais de 30cm, adicione resistores de **4.7kΩ** entre SDA e 3.3V, e entre SCL e 3.3V (pull-ups externos).
- O endereço I2C padrão é **0x20**. Se usar mais de um módulo, cada um precisa de um endereço diferente (0x20 a 0x27).

### Conexão do sinal de Interrupt (INT)

Além dos 4 fios I2C, há um quinto fio que permite ao módulo **avisar o principal quando um controle muda** — sem esperar que o principal pergunte:

```
    ESP32-C3                Módulo Principal
    ────────                ────────────────
    GPIO 10 (INT) ────────── GPIO 6 (INT)
```

Conexão:

1. **GPIO 10 do ESP32-C3** → fio de interrupt
2. **Outro lado do fio** → GPIO 6 do módulo principal

**Se usar múltiplos módulos**, todos os GPIO 10 vão no mesmo fio:

```
    Módulo #1 GPIO 10 ──┐
    Módulo #2 GPIO 10 ──┼──── Principal GPIO 6
    Módulo #3 GPIO 10 ──┘
```

Não precisa de resistor extra para cabos curtos (< 30cm). Para cabos mais longos, adicione um resistor de **4.7kΩ a 10kΩ** entre o fio INT e 3.3V, **apenas no módulo principal**.

> **Resumo dos fios necessários:** GND, 3.3V, SDA (GPIO 8), SCL (GPIO 9), INT (GPIO 10) = **5 fios**.

---

## Configurando o firmware para o seu hardware

Você só precisa editar **um arquivo**: `src/hardware/HardwareMap.h`

Abra o arquivo e encontre a lista `CONTROLES[]`. Cada linha representa um controle conectado à sua placa:

```cpp
constexpr ControleHW CONTROLES[] = {
    // {"Nome", GPIO, Tipo, CC_MIDI, Invertido, GPIO_B, GPIO_Switch}
    {"Pot1", 0, TipoControle::POTENCIOMETRO, 1, false, 0, 0},
    {"Btn1", 6, TipoControle::BOTAO, 2, false, 0, 0},
    {"Enc1", 18, TipoControle::ENCODER, 3, false, 19, 20},
};
```

### Explicação de cada campo

| Campo | O que é | Valores possíveis |
|-------|---------|-------------------|
| Nome | Apelido do controle (até 12 letras) | Qualquer texto curto: "Vol", "Filtro", "Play" |
| GPIO | Pino principal onde está conectado | 0–5 para pots/sensores, 0–10 ou 18–21 para botões/encoders |
| Tipo | Tipo de controle | `POTENCIOMETRO`, `BOTAO`, `SENSOR`, `ENCODER` |
| CC_MIDI | Número do Control Change MIDI | 1 a 127 (cada controle deve ter um número diferente) |
| Invertido | Inverte a direção do valor | `false` = normal, `true` = invertido |
| GPIO_B | Segundo pino do encoder (pino B) | Número do GPIO, ou `0` se não for encoder |
| GPIO_Switch | Pino do push-button do encoder | Número do GPIO, ou `0` se não tiver |

### Exemplos práticos

**4 potenciômetros + 2 botões:**

```cpp
constexpr ControleHW CONTROLES[] = {
    {"Volume",  0, TipoControle::POTENCIOMETRO, 7,  false, 0, 0},
    {"Filtro",  1, TipoControle::POTENCIOMETRO, 74, false, 0, 0},
    {"Reverb",  2, TipoControle::POTENCIOMETRO, 91, false, 0, 0},
    {"Delay",   3, TipoControle::POTENCIOMETRO, 92, false, 0, 0},
    {"Play",    6, TipoControle::BOTAO,         20, false, 0, 0},
    {"Stop",    7, TipoControle::BOTAO,         21, false, 0, 0},
};
```

**2 encoders com push-button + 1 pot:**

```cpp
constexpr ControleHW CONTROLES[] = {
    {"Pan",     0,  TipoControle::POTENCIOMETRO, 10, false, 0,  0},
    {"EncVol",  18, TipoControle::ENCODER,       7,  false, 19, 20},
    {"EncFx",   6,  TipoControle::ENCODER,       74, false, 7,  10},
};
```

**Módulo só de botões (8 botões):**

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

---

## Configurações opcionais

### Alterar o endereço I2C

No arquivo `src/config.h`, altere a linha:

```cpp
constexpr uint8_t I2C_ADDRESS = 0x20;
```

Valores válidos: `0x20` a `0x27` (até 8 módulos no mesmo barramento).

### Alterar o nome do módulo

No mesmo arquivo `src/config.h`:

```cpp
constexpr const char *MODULE_NAME = "EXT-01";
```

Dê um nome que identifique o módulo (ex: "FADERS", "DRUMS", "KNOBS").

---

## Usando multiplexador analógico (opcional)

Se precisar de mais de 6 potenciômetros, você pode usar um chip multiplexador (CD4051 para 8 canais ou CD4067 para 16 canais).

### Conexão do CD4051 (8 canais)

```
    ESP32-C3              CD4051
    ────────              ──────
    GPIO (ADC) ────────── SIG (pino 3)
    GPIO (digital) ────── S0 (pino 11)
    GPIO (digital) ────── S1 (pino 10)
    GPIO (digital) ────── S2 (pino 9)
    3.3V ─────────────── VCC (pino 16)
    GND ──────────────── GND (pino 8) + INH (pino 6) + VEE (pino 7)
```

Os 8 potenciômetros conectam seus cursores nos pinos Y0–Y7 do CD4051.

### Configuração no firmware

Edite o arquivo `src/hardware/AnalogMux.h`:

```cpp
constexpr MuxConfig MUX_CONFIG[] = {
    // {GPIO_sinal, S0, S1, S2, S3, num_canais}
    {5, 6, 7, 10, 0, 8},  // CD4051: sinal no GPIO 5, seleção nos GPIOs 6/7/10
};
```

| Campo | O que é |
|-------|---------|
| GPIO_sinal | Pino ADC onde o SIG do mux está conectado |
| S0, S1, S2 | Pinos digitais de seleção de canal |
| S3 | Pino S3 (só para CD4067 com 16 canais, use `0` para CD4051) |
| num_canais | `8` para CD4051, `16` para CD4067 |

---

## Gravando o firmware

1. Instale o [PlatformIO](https://platformio.org/install) (extensão do VS Code)
2. Abra a pasta do projeto no VS Code
3. Conecte o ESP32-C3 via USB
4. Clique no botão **Upload** (seta para direita) na barra inferior do VS Code
5. Aguarde a compilação e gravação

Se a configuração de GPIOs estiver errada, o firmware **não vai compilar** e mostrará uma mensagem de erro indicando o problema. Isso é intencional — protege contra erros de fiação.

---

## Limites do sistema

| Parâmetro | Limite |
|-----------|--------|
| Máximo de controles por módulo | 16 |
| Entradas analógicas diretas (sem mux) | 6 |
| Entradas analógicas com 1 mux CD4051 | 8 extras |
| Entradas analógicas com 1 mux CD4067 | 16 extras |
| Módulos no mesmo barramento I2C | 8 (endereços 0x20–0x27) |
| Comprimento máximo do cabo I2C | ~30cm sem pull-up externo, ~1-2m com pull-up de 4.7kΩ |

---

## Dicas de montagem

1. **Teste um controle por vez** — conecte um pot ou botão, grave o firmware, verifique se funciona, depois adicione os próximos.

2. **Potenciômetros baratos** podem não chegar a 0 ou 127 nas extremidades. O firmware tem calibração automática que resolve isso (ativada pelo módulo principal).

3. **Cabos longos em encoders** podem causar leituras erradas. Mantenha os fios do encoder curtos (< 15cm).

4. **Se um pot for desconectado** durante o uso, o firmware detecta automaticamente e congela o último valor válido (não envia lixo).

5. **Capacitor de 100nF** entre 3.3V e GND, próximo ao ESP32-C3, ajuda a reduzir ruído nas leituras analógicas.

6. **Não use GPIO 8 e 9** para controles — são reservados para comunicação I2C com o módulo principal.

---

## Resolução de problemas

| Problema | Causa provável | Solução |
|----------|---------------|---------|
| Firmware não compila | GPIO inválido na configuração | Verifique se pots usam GPIO 0–5 e botões/encoders usam GPIO 0–10 ou 18–21 |
| Pot não chega a 0 ou 127 | Não-linearidade do ADC | Use a calibração via módulo principal |
| Valor do pot oscila parado | Ruído elétrico | Adicione capacitor de 100nF entre o cursor e GND |
| Botão dispara sozinho | Fio solto ou sem GND | Verifique a conexão com GND |
| Encoder pula valores | Fios longos ou interferência | Encurte os fios, adicione capacitores de 100nF nos pinos A e B |
| Módulo não responde via I2C | Endereço duplicado ou sem pull-up | Verifique endereço único e adicione pull-ups de 4.7kΩ se cabo > 30cm |
| Módulo reinicia sozinho | Watchdog ativado (travamento) | Normal — o módulo se recupera automaticamente |
