# Módulo de Extensão MIDI — Análise de Produto e Melhorias

## Levantamento de Hardware do Módulo

### Microcontrolador
| Componente | Especificação |
|---|---|
| MCU | ESP32-C3 (esp32-c3-devkitc-02) |
| Arquitetura | RISC-V single-core, 160 MHz |
| Flash | 4 MB |
| RAM | 400 KB (SRAM) |
| ADC | 12-bit, 6 canais (GPIO 0-5) |
| GPIOs disponíveis | 0-10, 18-21 (total ~15 utilizáveis) |
| I2C | 1x hardware I2C (pinos configuráveis) |
| Alimentação | 3.3V (via regulador onboard no devkit) |

### Barramento de Comunicação
| Item | Detalhe |
|---|---|
| Protocolo | I2C (escravo) |
| Endereço | 0x20 (configurável na faixa 0x20-0x27) |
| Pinos | SDA = GPIO 8, SCL = GPIO 9 |
| Velocidade | Standard mode (100 kHz, padrão Wire) |
| Buffer | 256 bytes (configurado via Wire.setBufferSize) |

### Controles Suportados
| Tipo | Pinos por unidade | Circuito típico |
|---|---|---|
| Potenciômetro | 1 (ADC) | Divisor resistivo 10kΩ linear |
| Botão | 1 (digital + pullup interno) | Normalmente aberto, pull-up interno |
| Sensor | 1 (ADC) | Variável (FSR, LDR, etc.) |
| Encoder rotativo | 2 (A + B, pullup interno) | Quadratura mecânica com detent |

### Limites Atuais
| Parâmetro | Valor |
|---|---|
| Máximo de controles | 16 |
| GPIOs ADC disponíveis | 6 (GPIO 0-5) |
| GPIOs digitais disponíveis | ~15 |
| Resolução ADC | 12 bits → mapeado para 7 bits (MIDI) |
| Taxa de polling | Limitada pela velocidade do loop (~1-2 kHz estimado) |

---

## Melhorias Propostas

As melhorias estão separadas em **Software** (implementáveis agora, sem alterar hardware) e **Hardware** (dependem de PCB/BOM). O módulo principal já fornece feedback visual para toda a cadeia, então este módulo foca em ser confiável, invisível e barato.

---

## Software (Firmware)

### 🔴 Prioridade Crítica

#### 1. Watchdog Timer (WDT)
**Problema**: Se o firmware travar, o módulo para de responder indefinidamente. O mestre não tem como distinguir travamento de desconexão.
**Melhoria**: Ativar o WDT do ESP32-C3 (~5s timeout) para reiniciar automaticamente em caso de travamento.
**Esforço**: Baixo (3-5 linhas de código).
**Impacto**: O módulo se recupera sozinho de qualquer falha inesperada.

#### 2. Timeout no barramento I2C
**Problema**: Se o barramento I2C travar (clock stretching infinito, SDA preso em LOW), o módulo fica pendurado sem recuperação.
**Melhoria**: Configurar `Wire.setTimeOut()` e implementar detecção de bus stuck com recovery (toggle SCL para liberar SDA).
**Esforço**: Baixo.
**Impacto**: Robustez em ambientes com cabos longos ou interferência.

#### 3. Validação de GPIOs em tempo de compilação
**Problema**: Se o usuário configurar um GPIO inválido no HardwareMap, o comportamento é indefinido em runtime.
**Melhoria**: Adicionar `static_assert` para cada GPIO, validando contra a lista de GPIOs válidos do ESP32-C3 (ADC: 0-5; Digital: 0-10, 18-21).
**Esforço**: Baixo.
**Impacto**: Erros de configuração detectados antes de gravar — evita debug frustrante.

---

### 🟡 Prioridade Alta

#### 4. Suavização (smoothing) para leituras analógicas
**Problema**: Potenciômetros baratos geram jitter mesmo com deadzone. O valor MIDI oscila ±1-2 unidades em repouso.
**Melhoria**: Implementar média móvel exponencial (EMA) com fator configurável antes do mapeamento ADC→MIDI. Algo como `filtered = alpha * raw + (1-alpha) * filtered`.
**Esforço**: Médio (novo estado por controle analógico, ~20 linhas).
**Impacto**: Valores estáveis sem sacrificar responsividade. Diferença perceptível para o usuário.

#### 5. Calibração de ADC por canal
**Problema**: O ADC do ESP32-C3 tem não-linearidade nas extremidades. Potenciômetros podem não atingir 0 ou 127 nas posições extremas.
**Melhoria**: 
- Armazenar `adc_min` e `adc_max` por canal na NVS (flash)
- Modo de calibração ativado por comando I2C do mestre (CMD_CALIBRATE = 0x04)
- Mapear [adc_min, adc_max] → [0, 127] ao invés de [0, 4095] → [0, 127]
**Esforço**: Médio-alto (NVS + novo comando + lógica de calibração).
**Impacto**: Controles respondem full-range independente da qualidade do pot.

#### 6. Detecção de pino flutuante / controle desconectado
**Problema**: Se um potenciômetro for desconectado, o GPIO fica flutuando e gera valores aleatórios que são enviados ao mestre.
**Melhoria**: Detectar variância alta em janela curta (ex: >30 unidades MIDI em 50ms) e congelar o último valor válido. Reportar status de "desconectado" ao mestre via flag no descriptor.
**Esforço**: Médio.
**Impacto**: Evita envio de lixo ao mestre. Importante para uso em palco onde cabos podem soltar.

#### 7. Comando de identificação/versão (CMD_INFO = 0x03)
**Problema**: O mestre não sabe a versão do firmware nem consegue distinguir módulos além do endereço.
**Melhoria**: Novo comando que retorna: versão firmware (major.minor.patch), MODULE_NAME, chip ID único do ESP32-C3.
**Esforço**: Baixo.
**Impacto**: Permite ao mestre verificar compatibilidade de protocolo e logar qual módulo está em qual endereço.

---

### 🟢 Prioridade Média

#### 8. Suporte a encoder com push-button integrado
**Problema**: Encoders EC11 (os mais comuns e baratos) têm push-button integrado. O firmware atual não trata o push como controle associado.
**Melhoria**: Adicionar campo `gpioSwitch` ao ControleHW. Quando tipo == ENCODER e gpioSwitch != 0, tratar como botão com debounce independente, ocupando o slot seguinte no buffer.
**Esforço**: Médio.
**Impacto**: Suporta o encoder mais popular do mercado sem gambiarras.

#### 9. Persistência de configuração em NVS
**Problema**: Deadzone, debounce, inversão — tudo é fixo em compilação. Para ajustar, precisa recompilar.
**Melhoria**: Permitir que o mestre envie configurações via I2C (CMD_SET_CONFIG = 0x05) que são salvas na NVS e carregadas no boot.
**Esforço**: Alto.
**Impacto**: Personalização sem recompilação. Útil se vender módulos pré-montados.

#### 10. Modo de baixo consumo
**Problema**: Se o mestre for desligado, o módulo continua consumindo ~40mA sem necessidade.
**Melhoria**: Entrar em light sleep após N segundos sem requisição I2C. Acordar por interrupção no barramento (address match).
**Esforço**: Médio.
**Impacto**: Relevante se alimentado por bateria ou USB do mestre.

#### 11. Aceleração de encoder
**Problema**: Encoders com detent (24 pulsos/volta) são lentos para percorrer 0-127. O usuário precisa girar ~5 voltas completas.
**Melhoria**: Detectar velocidade de rotação (pulsos/tempo) e multiplicar o incremento quando rápido (ex: +1 lento, +4 médio, +8 rápido).
**Esforço**: Médio.
**Impacto**: Usabilidade muito melhor para encoders. Padrão em controladores MIDI comerciais.

#### 12. Suporte a multiplexador analógico (CD4051/CD4067)
**Problema**: O ESP32-C3 tem apenas 6 canais ADC. Módulos com muitos pots ficam limitados.
**Melhoria**: Suportar mux analógico: 3 GPIOs de seleção + 1 GPIO ADC = 8 canais extras por mux. Configurável no HardwareMap.
**Esforço**: Alto.
**Impacto**: Permite módulos com até 16 potenciômetros usando apenas 4 GPIOs.

---

### 🔵 Prioridade Baixa (Nice-to-have)

#### 13. OTA update via mestre I2C
**Problema**: Atualizar firmware requer acesso USB direto ao ESP32-C3.
**Melhoria**: Protocolo de atualização via I2C onde o mestre envia blocos do firmware novo.
**Esforço**: Muito alto (bootloader custom, protocolo de transferência, verificação CRC).
**Impacto**: Atualização sem abrir gabinete. Luxo para produto final.

#### 14. Rate limiting no loop de leitura
**Problema**: O loop roda o mais rápido possível (~1-2 kHz). Para botões e encoders isso é bom, mas para ADC é desperdício de CPU e pode aumentar ruído.
**Melhoria**: Ler analógicos a cada N ms (ex: 5ms = 200Hz) e digitais a cada iteração.
**Esforço**: Baixo.
**Impacto**: Menor consumo, leituras ADC mais estáveis.

---

## Hardware (PCB/BOM)

### 🟡 Prioridade Alta

#### H1. Configuração de endereço I2C via DIP switch
**Problema**: Endereço fixo em compilação. Múltiplos módulos exigem firmware diferente.
**Melhoria**: 3 GPIOs + DIP switch de 3 posições → endereço 0x20-0x27 selecionável fisicamente.
**Impacto**: Até 8 módulos sem recompilação.

#### H2. Pull-ups externos no I2C (4.7kΩ)
**Problema**: Pull-ups internos do ESP32-C3 são fracos (~45kΩ). Com cabos >30cm o sinal degrada.
**Melhoria**: Resistores 4.7kΩ em SDA e SCL na PCB.
**Impacto**: Comunicação confiável com cabos de até 1-2m.

#### H3. Conector padronizado (JST-SH 4 pinos / Qwiic)
**Problema**: Sem conector definido, cada montagem é diferente.
**Melhoria**: Adotar Qwiic/STEMMA QT (JST-SH 4 pinos: GND, 3.3V, SDA, SCL). Padrão da indústria maker.
**Impacto**: Plug-and-play, sem solda para o barramento.

---

### 🟢 Prioridade Média

#### H4. Proteção ESD nos GPIOs expostos
**Problema**: Hot-plug de controles pode gerar ESD.
**Melhoria**: Diodos TVS ou resistores de série (100-330Ω) nos GPIOs de controle.
**Impacto**: Proteção contra danos por manuseio.

#### H5. Capacitores de desacoplamento
**Problema**: Ruído na alimentação pode afetar leituras ADC.
**Melhoria**: 100nF cerâmico próximo ao VCC do ESP32-C3 + 10µF eletrolítico na entrada de alimentação.
**Impacto**: Leituras ADC mais limpas, menos susceptibilidade a ruído.

#### H6. PCB com módulo ESP32-C3 soldável (não devkit)
**Problema**: O devkit é grande e caro para produção em série.
**Melhoria**: Usar módulo ESP32-C3-MINI-1 soldado diretamente na PCB custom.
**Impacto**: Menor custo, menor tamanho, mais profissional.

---

## Resumo

| Categoria | Qtd | Implementáveis agora (SW) |
|---|---|---|
| Software — Crítico | 3 | ✅ Todas |
| Software — Alto | 4 | ✅ Todas |
| Software — Médio | 5 | ✅ Todas |
| Software — Baixo | 2 | ✅ Todas |
| Hardware | 6 | ❌ Dependem de PCB |

**Total: 14 melhorias de software + 6 de hardware.**

## Ordem Sugerida de Implementação (Software)

1. Watchdog + timeout I2C + validação GPIO (proteção básica)
2. Smoothing + calibração ADC (qualidade dos controles)
3. Detecção de pino flutuante (robustez em uso real)
4. CMD_INFO (protocolo completo)
5. Encoder push-button + aceleração (compatibilidade)
6. NVS + rate limiting (refinamento)
7. Multiplexador + OTA (expansão futura)
