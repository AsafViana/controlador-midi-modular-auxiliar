#include "config.h"
#include "hardware/AnalogMux.h"
#include "hardware/Calibration.h"
#include "hardware/ControlReader.h"
#include "hardware/PersistentConfig.h"
#include "i2c/I2CSlave.h"
#include <Arduino.h>
#include <esp_sleep.h>
#include <esp_task_wdt.h>

static uint32_t lastActivityMs = 0;

void setup() {
  // Inicializa Watchdog Timer
  const esp_task_wdt_config_t wdtConfig = {
      .timeout_ms = WDT_TIMEOUT_MS,
      .idle_core_mask = 0,  // Não monitora idle tasks
      .trigger_panic = true // Reinicia o MCU em caso de timeout
  };
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL); // Adiciona a task atual (loopTask) ao WDT

  PersistentConfig::init();
  Calibration::init();
  AnalogMux::init();
  ControlReader::init();
  I2CSlave::init();

  lastActivityMs = millis();
}

void loop() {
  ControlReader::update();
  esp_task_wdt_reset(); // Alimenta o watchdog a cada iteração

  // Verifica se OTA completou e restart está pendente
  if (I2CSlave::isOtaRestartPending()) {
    delay(100); // Aguarda I2C finalizar transmissão
    ESP.restart();
  }

  // Verifica se houve atividade I2C recente
  if (I2CSlave::hasRecentActivity()) {
    lastActivityMs = millis();
  }

  // Entra em light sleep se inativo por SLEEP_TIMEOUT_MS
  if ((millis() - lastActivityMs) >= SLEEP_TIMEOUT_MS) {
    esp_task_wdt_delete(NULL); // Remove WDT antes de dormir

    // Configura wake-up por GPIO (SDA - atividade I2C)
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable((gpio_num_t)PIN_SDA, GPIO_INTR_LOW_LEVEL);

    esp_light_sleep_start();

    // Acordou — reinicializa WDT
    esp_task_wdt_add(NULL);
    lastActivityMs = millis();
  }
}
