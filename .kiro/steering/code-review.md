# Code Review Process — Firmware

## Princípios

1. **Segurança primeiro** — firmware embarcado não tem rollback fácil
2. **Recursos são finitos** — RAM, flash, ciclos de CPU, GPIOs
3. **Determinismo** — código deve ter comportamento previsível e tempo-real
4. **Simplicidade** — menos código = menos bugs em embarcado

## Prioridade de review

1. **Segurança elétrica** — GPIOs corretos, sem conflito com I2C
2. **Robustez** — watchdog, detecção de erros, recovery
3. **Correção** — lógica de leitura, mapeamento ADC→MIDI, protocolo I2C
4. **Performance** — não bloquear loop, rate limiting, consumo
5. **Legibilidade** — nomes claros, comentários em pontos não-óbvios
6. **Estilo** — formatação consistente

## Tamanho de PR

- **Ideal**: até 200 linhas alteradas
- **Máximo aceitável**: 400 linhas
- **Acima de 400**: dividir (exceto refactors mecânicos)

## Checklist do reviewer

### Obrigatório verificar

- [ ] GPIOs usados são válidos para o tipo de controle
- [ ] Sem conflito com pinos reservados (GPIO 8, 9)
- [ ] Watchdog é alimentado em todos os paths do loop
- [ ] Sem alocação dinâmica em código de runtime
- [ ] Sem `delay()` no loop principal
- [ ] Buffer I2C não excede 256 bytes
- [ ] Constantes em `config.h` (não hardcoded)
- [ ] `static_assert` para validações de configuração
- [ ] Compila sem warnings (`-Wall -Wextra`)
- [ ] Testes nativos passam para lógica nova

### Desejável verificar

- [ ] Suavização ADC aplicada corretamente
- [ ] Debounce adequado para entradas digitais
- [ ] Nomes descritivos para variáveis e funções
- [ ] Comentários em trechos de hardware não-óbvios
- [ ] Sem uso desnecessário de float
- [ ] Rate limiting respeitado para ADC

## Etiqueta

### Para quem abre o PR
- Descrever qual hardware/controle foi afetado
- Se alterou HardwareMap: descrever a configuração testada
- Indicar se testou em hardware real ou apenas compilou

### Para quem revisa
- Prefixos:
  - `[blocker]` — bug de hardware, pode danificar componente ou travar
  - `[safety]` — risco de undefined behavior ou corrupção de dados
  - `[suggestion]` — melhoria opcional
  - `[nit]` — estilo, não bloqueia
