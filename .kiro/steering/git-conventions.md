# Git Conventions

## Branching Strategy

### Branches principais
- `main` — versão estável, testada em hardware

### Branches de trabalho
```
feature/descricao-curta
bugfix/descricao-curta
hotfix/descricao-curta
release/v0.4.0
```

### Regras
- Sempre criar branch a partir de `main`
- Deletar branch após merge
- Nunca push direto em `main`

## Commits

### Formato: Conventional Commits
```
tipo(escopo): descrição curta

corpo opcional explicando o porquê
```

### Tipos permitidos
| Tipo | Quando usar |
|------|-------------|
| `feat` | Nova funcionalidade (novo controle, novo comando I2C) |
| `fix` | Correção de bug |
| `refactor` | Refatoração sem mudança de comportamento |
| `test` | Adição ou correção de testes |
| `docs` | Documentação |
| `chore` | Manutenção (deps, configs, PlatformIO) |
| `perf` | Melhoria de performance/consumo |
| `hw` | Alteração relacionada a hardware (GPIOs, pinout, mux) |

### Escopos comuns
| Escopo | Módulo |
|--------|--------|
| `hardware` | ControlReader, HardwareMap, Calibration, AnalogMux |
| `i2c` | I2CSlave, protocolo de comunicação |
| `ota` | Atualização de firmware via I2C |
| `config` | config.h, PersistentConfig (NVS) |
| `build` | platformio.ini, scripts de build |

### Regras de commit
- Descrição em português
- Máximo 72 caracteres na primeira linha
- Imperativo: "adiciona suporte a encoder" (não "adicionado")
- Um commit = uma mudança lógica
- Nunca commitar: `.pio/`, `.cache/`, `compile_commands.json`

## Pull Requests

### Título
```
tipo(escopo): descrição curta
```

### Descrição (template)
```markdown
## O que foi feito
Breve descrição da mudança.

## Hardware afetado
Quais controles/GPIOs/protocolo foram alterados.

## Como testar
- [ ] Compilou sem warnings
- [ ] Testes nativos passam (`pio test -e native`)
- [ ] Testado em hardware (descrever setup)

## Checklist
- [ ] config.h atualizado se necessário
- [ ] static_assert adicionado para novos GPIOs
- [ ] Documentação atualizada (README/docs)
```

### Merge strategy
- **Squash merge** para features (histórico limpo)
- **Merge commit** para releases
