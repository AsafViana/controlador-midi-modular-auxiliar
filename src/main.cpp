#include "hardware/ControlReader.h"
#include "i2c/I2CSlave.h"
#include <Arduino.h>

void setup() {
  ControlReader::init();
  I2CSlave::init();
}

void loop() { ControlReader::update(); }
