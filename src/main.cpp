#include "config.h"
#include "hardware/ControlReader.h"
#include "i2c/I2CSlave.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

void setup() {
  // Inicializa Watchdog Timer
  const esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = WDT_TIMEOUT_MS,
      .idle_core_mask = 0,  // Não monitora idle tasks
      .trigger_panic = true // Reinicia o MCU em caso de timeout
  };
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL); // Adiciona a task atual (loopTask) ao WDT

  ControlReader::init();
  I2CSlave::init();
}

void loop() {
  ControlReader::update();
  esp_task_wdt_reset(); // Alimenta o watchdog a cada iteração
}
