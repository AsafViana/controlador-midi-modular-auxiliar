# Quality Gates — Firmware

## Gate 1: Compilação (automático)

- [ ] Build compila sem erros (`pio run`)
- [ ] Zero warnings com `-Wall -Wextra`
- [ ] `static_assert` passa para todas as configurações de GPIO
- [ ] Tamanho do firmware dentro do limite (< 1.5 MB de flash usado)
- [ ] RAM estática não excede 80% do disponível (320 KB de 400 KB)

## Gate 2: Testes Nativos

- [ ] Todos os testes nativos passam (`pio test -e native`)
- [ ] Propriedades RapidCheck verificadas:
  - Mapeamento ADC → MIDI sempre no range [0, 127]
  - EMA nunca produz valor fora do range
  - Debounce não perde eventos reais
  - Encoder não overflow em rotação contínua
  - HardwareMap com 16 controles não excede buffer I2C

## Gate 3: Validação em Hardware

- [ ] Upload bem-sucedido no ESP32-C3
- [ ] Módulo responde a CMD_INFO via I2C
- [ ] Potenciômetro varre 0-127 corretamente
- [ ] Botão responde com debounce (sem disparo duplo)
- [ ] Encoder incrementa/decrementa suavemente
- [ ] Aceleração de encoder funciona (giro rápido = saltos maiores)
- [ ] Módulo entra em light sleep após 30s sem atividade
- [ ] Módulo acorda ao receber requisição I2C
- [ ] Watchdog reinicia módulo se loop travar (teste com `while(1)`)

## Gate 4: Robustez

- [ ] Desconectar potenciômetro → valor congela (não envia lixo)
- [ ] Cabo I2C longo (>30cm) com pull-ups → comunicação estável
- [ ] 16 controles simultâneos → resposta <10ms por read
- [ ] Reiniciar módulo principal → escravo recupera automaticamente
- [ ] Múltiplos módulos no barramento → sem conflito de endereço

## Métricas monitoradas

| Métrica | Threshold | Ação se violado |
|---------|-----------|-----------------|
| Tamanho firmware | > 1.5 MB | Investigar — otimizar |
| RAM estática | > 320 KB | Bloqueia merge |
| Tempo de loop | > 5ms | Investigar bloqueios |
| Jitter ADC (com EMA) | > ±1 MIDI | Ajustar alpha |
| Latência I2C response | > 10ms | Bug — investigar |
| Warnings de compilação | > 0 | Bloqueia merge |

## Exceções

- **Hotfix crítico** (módulo travando em campo): pode pular Gate 2, mas exige teste em hardware (Gate 3)
- **Mudança apenas em docs/README**: pula todos os gates de hardware
