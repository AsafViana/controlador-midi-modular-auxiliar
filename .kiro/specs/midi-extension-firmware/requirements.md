# Requirements Document

## Introduction

Firmware para módulo de extensão de um controlador MIDI modular. Este módulo opera como escravo I2C, lendo entradas de controle (botões, potenciômetros, sensores, encoders) e servindo os valores ao módulo principal sob demanda. Não possui display, saída MIDI direta ou armazenamento — toda interação adicional é feita pelo módulo principal.

Hardware alvo: ESP32-C3 (esp32-c3-devkitc-02)

## Glossary

- **Módulo_Extensão**: O firmware do módulo escravo I2C que lê controles e responde ao módulo principal
- **Módulo_Principal**: O controlador MIDI principal que atua como mestre I2C e solicita dados do Módulo_Extensão
- **HardwareMap**: Namespace que define o mapeamento de GPIOs, labels e tipos de cada controle físico do módulo
- **ControleHW**: Struct que descreve um controle de hardware (label, GPIO, tipo, CC padrão, flag invertido)
- **TipoControle**: Enum que classifica controles em BOTAO, POTENCIOMETRO, SENSOR ou ENCODER
- **ModuleDescriptor**: Estrutura serializada que descreve os controles do módulo (tipo + label + valor por controle)
- **CMD_DESCRIPTOR**: Comando I2C (0x01) que solicita o descritor do módulo
- **CMD_READ_VALUES**: Comando I2C (0x02) que solicita os valores atuais de todos os controles
- **DeadZone**: Faixa de tolerância para leituras analógicas que evita oscilações por ruído
- **Barramento_I2C**: Interface de comunicação I2C entre Módulo_Extensão (escravo) e Módulo_Principal (mestre)
- **ControlReader**: Componente responsável por ler e processar os valores dos controles de hardware

## Requirements

### Requirement 1: Organização do Projeto em Componentes

**User Story:** Como desenvolvedor, quero que o firmware do módulo de extensão siga a mesma organização por componentes do firmware principal, para que a manutenção e navegação sejam consistentes entre os projetos.

#### Acceptance Criteria

1. THE Módulo_Extensão SHALL organizar o código-fonte em pastas separadas por responsabilidade: `hardware/`, `i2c/`, e `config.h` na raiz de `src/`
2. THE Módulo_Extensão SHALL manter um arquivo `main.cpp` como ponto de entrada com funções `setup()` e `loop()`
3. THE Módulo_Extensão SHALL separar a lógica de leitura de controles no componente `hardware/`
4. THE Módulo_Extensão SHALL separar a lógica de comunicação I2C escravo no componente `i2c/`

### Requirement 2: Mapeamento de Hardware (HardwareMap)

**User Story:** Como desenvolvedor, quero um mapa de GPIOs com nomes descritivos para cada controle, para que eu possa facilmente identificar e configurar os controles físicos do módulo.

#### Acceptance Criteria

1. THE HardwareMap SHALL definir cada controle usando a struct ControleHW com campos: label (const char*, max 12 caracteres), gpio (uint8_t), tipo (TipoControle), ccPadrao (uint8_t) e invertido (bool)
2. THE HardwareMap SHALL expor um array constexpr CONTROLES contendo todos os controles definidos
3. THE HardwareMap SHALL expor uma constante NUM_CONTROLES calculada automaticamente a partir do tamanho do array CONTROLES
4. THE HardwareMap SHALL fornecer funções auxiliares: getLabel(), getGpio(), getTipo(), isInvertido() e isAnalogico()
5. THE HardwareMap SHALL limitar o número máximo de controles a 16 por módulo
6. THE HardwareMap SHALL definir o enum TipoControle com os valores: BOTAO, POTENCIOMETRO, SENSOR e ENCODER

### Requirement 3: Leitura de Controles

**User Story:** Como módulo de extensão, quero ler continuamente os valores dos controles de hardware, para que os valores estejam sempre atualizados quando o módulo principal os solicitar.

#### Acceptance Criteria

1. WHILE o Módulo_Extensão estiver em operação, THE ControlReader SHALL ler os valores de todos os controles definidos no HardwareMap a cada iteração do loop principal
2. WHEN um controle do tipo POTENCIOMETRO ou SENSOR for lido, THE ControlReader SHALL aplicar uma DeadZone para filtrar oscilações de ruído na leitura analógica
3. WHEN um controle do tipo BOTAO for lido, THE ControlReader SHALL aplicar debounce para evitar leituras falsas
4. THE ControlReader SHALL mapear leituras analógicas do ADC (0-4095 no ESP32-C3) para o intervalo MIDI (0-127)
5. WHEN um controle tiver a flag invertido ativa, THE ControlReader SHALL inverter o valor lido (127 - valor para analógicos, inversão lógica para digitais)
6. THE ControlReader SHALL armazenar os valores processados em um buffer acessível pelo componente I2C

### Requirement 4: Comunicação I2C Escravo

**User Story:** Como módulo de extensão, quero responder a comandos I2C do módulo principal, para que ele possa descobrir meus controles e ler seus valores.

#### Acceptance Criteria

1. WHEN o Módulo_Extensão inicializar, THE Barramento_I2C SHALL registrar o módulo como escravo no endereço configurado em config.h (faixa 0x20-0x27)
2. WHEN o Módulo_Principal enviar CMD_DESCRIPTOR (0x01), THE Módulo_Extensão SHALL responder com o ModuleDescriptor serializado contendo tipo, label e valor atual de cada controle
3. WHEN o Módulo_Principal enviar CMD_READ_VALUES (0x02), THE Módulo_Extensão SHALL responder com os valores atuais de todos os controles (1 byte por controle, na ordem do HardwareMap)
4. THE Módulo_Extensão SHALL serializar cada controle no ModuleDescriptor como: tipo (1 byte) + label (12 bytes, preenchido com zeros) + valor (1 byte), totalizando 14 bytes por controle
5. IF um comando desconhecido for recebido, THEN THE Módulo_Extensão SHALL ignorar o comando sem alterar seu estado
6. THE Módulo_Extensão SHALL responder ao CMD_DESCRIPTOR com o cabeçalho: numControles (1 byte) seguido dos dados de cada controle

### Requirement 5: Configuração do Módulo

**User Story:** Como desenvolvedor, quero um arquivo de configuração centralizado com as constantes do módulo, para que eu possa ajustar parâmetros sem modificar a lógica do código.

#### Acceptance Criteria

1. THE Módulo_Extensão SHALL definir em config.h o endereço I2C do módulo (padrão: 0x20)
2. THE Módulo_Extensão SHALL definir em config.h o nome do módulo (string identificadora)
3. THE Módulo_Extensão SHALL definir em config.h os pinos SDA e SCL para o barramento I2C
4. THE Módulo_Extensão SHALL definir em config.h o valor da DeadZone para leituras analógicas
5. THE Módulo_Extensão SHALL definir em config.h o tempo de debounce para botões (em milissegundos)

### Requirement 6: Inicialização do Sistema

**User Story:** Como módulo de extensão, quero que a inicialização configure todos os periféricos corretamente, para que o módulo esteja pronto para operar assim que ligado.

#### Acceptance Criteria

1. WHEN o Módulo_Extensão inicializar, THE Módulo_Extensão SHALL configurar todos os GPIOs definidos no HardwareMap com o modo apropriado (INPUT, INPUT_PULLUP para botões, ANALOG para potenciômetros/sensores)
2. WHEN o Módulo_Extensão inicializar, THE Módulo_Extensão SHALL inicializar o barramento I2C no modo escravo com o endereço definido em config.h
3. WHEN o Módulo_Extensão inicializar, THE Módulo_Extensão SHALL registrar os callbacks de recepção e solicitação I2C
4. WHEN o Módulo_Extensão inicializar, THE Módulo_Extensão SHALL inicializar o buffer de valores de controle com zeros

### Requirement 7: Leitura de Encoder Rotativo

**User Story:** Como módulo de extensão, quero suportar encoders rotativos, para que o módulo principal possa usar controles de rotação infinita.

#### Acceptance Criteria

1. WHEN um controle do tipo ENCODER for definido no HardwareMap, THE ControlReader SHALL monitorar os dois pinos (A e B) para detectar direção de rotação
2. WHEN o encoder rotacionar no sentido horário, THE ControlReader SHALL incrementar o valor do controle (limitado a 127)
3. WHEN o encoder rotacionar no sentido anti-horário, THE ControlReader SHALL decrementar o valor do controle (limitado a 0)
4. THE ControlReader SHALL inicializar o valor do encoder em 64 (ponto médio do intervalo MIDI)
