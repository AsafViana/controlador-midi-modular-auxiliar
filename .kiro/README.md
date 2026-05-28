# .kiro — Módulo de Extensão MIDI (ESP32-C3)

Este diretório contém a configuração do Kiro IDE para o firmware do módulo de extensão MIDI.

## Estrutura

```
.kiro/
├── steering/
│   ├── product.md          → Contexto do projeto (firmware MIDI I2C)
│   ├── tech.md             → Regras técnicas da stack (C++17, PlatformIO, ESP-IDF)
│   ├── rules.md            → Regras absolutas (NUNCA/SEMPRE) para firmware embarcado
│   ├── code-review.md      → Processo de review para código embarcado
│   ├── git-conventions.md  → Branching, commits, PRs
│   └── quality-gates.md    → Gates de qualidade para firmware
├── hooks/
│   ├── on-save.json        → Feedback ao salvar arquivos .cpp/.h
│   └── pre-commit.json     → Validação antes de escrever código
├── specs/
│   └── midi-extension-firmware/  → Especificações do firmware
├── settings/
│   └── lsp.json            → Configuração do clangd (LSP para C++)
└── README.md               → Este arquivo
```

## Stack

- **MCU**: ESP32-C3 (RISC-V, 160 MHz)
- **Framework**: Arduino (via PlatformIO)
- **Linguagem**: C++17
- **Build**: PlatformIO
- **Comunicação**: I2C Slave
- **Testes**: RapidCheck (property-based testing) em ambiente nativo

## Como usar

1. Abra o projeto no Kiro IDE
2. O steering é carregado automaticamente como contexto
3. Hooks de on-save validam padrões de código embarcado
4. Pre-commit previne erros comuns antes de escrever
