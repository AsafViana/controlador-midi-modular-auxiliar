# Regras Absolutas — Firmware Embarcado ESP32-C3

> Violação de qualquer regra abaixo = código rejeitado. Sem exceções.

## NUNCA

- ❌ NUNCA usar alocação dinâmica (`new`, `malloc`, `std::vector` sem tamanho fixo) — memória é limitada e fragmentação mata
- ❌ NUNCA usar `#define` para constantes — usar `constexpr`
- ❌ NUNCA usar `float`/`double` para cálculos que podem ser feitos em ponto fixo (ESP32-C3 não tem FPU)
- ❌ NUNCA usar `delay()` no loop principal — bloqueia leitura de controles e I2C
- ❌ NUNCA acessar GPIO 8 ou 9 para controles — reservados para I2C
- ❌ NUNCA configurar potenciômetro/sensor em GPIO > 5 — apenas GPIO 0-5 têm ADC
- ❌ NUNCA usar `int` genérico — usar tipos explícitos (`uint8_t`, `uint16_t`, `uint32_t`)
- ❌ NUNCA ignorar o watchdog — sempre chamar `esp_task_wdt_reset()` no loop
- ❌ NUNCA enviar dados I2C sem validar o tamanho do buffer (máx 256 bytes)
- ❌ NUNCA usar strings dinâmicas (`String`, `std::string`) em código que roda no loop
- ❌ NUNCA commitar código comentado
- ❌ NUNCA hardcodar endereço I2C fora de `config.h`
- ❌ NUNCA usar `Serial.print` em código de produção (não há USB serial em modo I2C slave)
- ❌ NUNCA ter mais de 16 controles no array `CONTROLES[]`
- ❌ NUNCA duplicar CC MIDI no mesmo módulo
- ❌ NUNCA usar GPIO sem verificar no HardwareMap se é válido para o tipo de controle

## SEMPRE

- ✅ SEMPRE usar `constexpr` para constantes (compile-time, zero RAM)
- ✅ SEMPRE validar configuração de GPIOs com `static_assert`
- ✅ SEMPRE usar `#pragma once` em headers
- ✅ SEMPRE alimentar o watchdog em cada iteração do loop
- ✅ SEMPRE usar tipos de tamanho explícito (`uint8_t`, `uint16_t`, etc.)
- ✅ SEMPRE aplicar suavização (EMA) antes de mapear ADC → MIDI
- ✅ SEMPRE verificar pino flutuante antes de enviar valor ao mestre
- ✅ SEMPRE incluir debounce em leituras de botão/encoder
- ✅ SEMPRE manter `config.h` como fonte única de verdade para constantes
- ✅ SEMPRE manter `HardwareMap.h` como único arquivo editável pelo usuário
- ✅ SEMPRE aplicar rate limiting nas leituras analógicas (≤200Hz)
- ✅ SEMPRE usar pull-up interno para botões e encoders (`INPUT_PULLUP`)
- ✅ SEMPRE documentar campos de structs com comentários inline
- ✅ SEMPRE testar edge cases (ADC=0, ADC=4095, encoder overflow, 16 controles)
- ✅ SEMPRE manter a compilação limpa: zero warnings com `-Wall -Wextra`
