# Implementation Plan: MIDI Extension Firmware

## Overview

Firmware para módulo de extensão I2C de um controlador MIDI modular, rodando em ESP32-C3. A implementação segue a ordem de dependência: configuração → mapeamento de hardware → leitura de controles (com funções puras testáveis) → comunicação I2C → integração no main.cpp → testes.

## Tasks

- [x] 1. Set up project structure and configuration
  - [x] 1.1 Create `src/config.h` with all module constants
    - Define I2C_ADDRESS (0x20), MODULE_NAME, PIN_SDA, PIN_SCL
    - Define DEADZONE, DEBOUNCE_MS, MAX_CONTROLES
    - Define CMD_DESCRIPTOR (0x01), CMD_READ_VALUES (0x02)
    - Define MIDI_MAX (127), MIDI_MID (64), ADC_MAX (4095)
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5_

  - [x] 1.2 Update `platformio.ini` with native test environment and RapidCheck dependency
    - Add `[env:native]` environment for host-based testing
    - Add RapidCheck library dependency for PBT
    - Configure test directory for native tests (`test/test_native/`)
    - _Requirements: 1.1_

  - [x] 1.3 Create directory structure for components
    - Create `src/hardware/` and `src/i2c/` directories
    - Create placeholder header files to establish structure
    - _Requirements: 1.1, 1.3, 1.4_

- [x] 2. Implement HardwareMap
  - [x] 2.1 Create `src/hardware/HardwareMap.h` with types and control definitions
    - Define `TipoControle` enum (BOTAO, POTENCIOMETRO, SENSOR, ENCODER)
    - Define `ControleHW` struct with fields: label, gpio, tipo, ccPadrao, invertido, gpioB
    - Define constexpr CONTROLES array with example controls
    - Define NUM_CONTROLES computed from array size
    - Implement helper functions: getLabel(), getGpio(), getTipo(), isInvertido(), isAnalogico(), getGpioB()
    - Add static_assert for MAX_CONTROLES limit (≤ 16)
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6_

- [x] 3. Implement ControlReader with pure processing functions
  - [x] 3.1 Create `src/hardware/ControlReader.h` with public interface
    - Declare init(), update(), getValues(), getValue(), getNumControles()
    - Declare pure processing functions for testability: mapAdcToMidi(), applyDeadzone(), applyDebounce(), invertValue(), processEncoderTransition()
    - _Requirements: 3.1, 3.6_

  - [x] 3.2 Implement pure processing functions in `src/hardware/ControlReader.cpp`
    - Implement `mapAdcToMidi(uint16_t adcValue)` → maps [0,4095] to [0,127]
    - Implement `applyDeadzone(uint8_t newValue, uint8_t lastValue, uint8_t deadzone)` → returns true if change exceeds threshold
    - Implement `applyDebounce(bool reading, bool lastStable, uint32_t lastChangeMs, uint32_t nowMs, uint16_t debounceMs)` → returns stable state
    - Implement `invertValue(uint8_t value)` → returns 127 - value
    - Implement `processEncoderTransition(uint8_t lastAB, uint8_t currentAB, uint8_t currentValue)` → returns new encoder value using quadrature table
    - _Requirements: 3.2, 3.3, 3.4, 3.5, 7.1, 7.2, 7.3_

  - [x] 3.3 Implement hardware interaction in `src/hardware/ControlReader.cpp`
    - Implement `init()` to configure GPIOs (INPUT_PULLUP for buttons, ANALOG for pots/sensors)
    - Implement `update()` loop that reads all controls and applies processing
    - Manage internal state: ButtonState (debounce), EncoderState (quadrature), AnalogState (deadzone)
    - Store processed values in volatile valueBuffer accessible by I2C component
    - Initialize encoder values at MIDI_MID (64)
    - _Requirements: 3.1, 3.2, 3.3, 3.5, 3.6, 6.1, 6.4, 7.4_

- [x] 4. Checkpoint - Verify ControlReader compiles
  - Ensure the project compiles for esp32-c3 target, ask the user if questions arise.

- [ ] 5. Implement I2CSlave
  - [x] 5.1 Create `src/i2c/I2CSlave.h` with public interface
    - Declare init() function
    - Declare pure serialization function: serializeDescriptor()
    - _Requirements: 4.1_

  - [x] 5.2 Implement serialization logic in `src/i2c/I2CSlave.cpp`
    - Implement `serializeDescriptor(const ControleHW* controls, const uint8_t* values, uint8_t count, uint8_t* outBuffer)` as pure function
    - Serialize format: numControles(1 byte) + [tipo(1) + label(12, zero-padded) + valor(1)] per control = 14 bytes per control
    - _Requirements: 4.2, 4.4, 4.6_

  - [x] 5.3 Implement I2C slave callbacks in `src/i2c/I2CSlave.cpp`
    - Implement `init()`: configure Wire as slave at I2C_ADDRESS, set buffer size to 256, register onReceive/onRequest callbacks
    - Implement `onReceive` callback: store lastCommand if valid (0x01 or 0x02), ignore unknown commands
    - Implement `onRequest` callback: respond with serialized descriptor for CMD_DESCRIPTOR, raw values for CMD_READ_VALUES
    - _Requirements: 4.1, 4.2, 4.3, 4.5, 4.6, 6.2, 6.3_

- [ ] 6. Implement main.cpp integration
  - [x] 6.1 Implement `setup()` in `src/main.cpp`
    - Call ControlReader::init() to configure GPIOs
    - Call I2CSlave::init() to start I2C slave
    - Initialize valueBuffer with zeros
    - _Requirements: 6.1, 6.2, 6.3, 6.4_

  - [x] 6.2 Implement `loop()` in `src/main.cpp`
    - Call ControlReader::update() each iteration
    - _Requirements: 3.1_

- [x] 7. Checkpoint - Full firmware compiles and links
  - Ensure the project compiles and links for esp32-c3 target with all components wired together, ask the user if questions arise.

- [ ] 8. Property-based tests for pure functions
  - [x] 8.1 Write property test for ADC to MIDI mapping
    - **Property 1: ADC to MIDI mapping preserves range and monotonicity**
    - Test that for any ADC value in [0, 4095], output is in [0, 127]
    - Test that for any a ≤ b, mapAdcToMidi(a) ≤ mapAdcToMidi(b)
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 3.4**

  - [x] 8.2 Write property test for deadzone filtering
    - **Property 2: Deadzone filters noise below threshold**
    - Test that output only changes when |newValue - lastValue| > DEADZONE
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 3.2**

  - [x] 8.3 Write property test for debounce stability
    - **Property 3: Debounce rejects unstable signals**
    - Test that output only transitions when input stable for ≥ DEBOUNCE_MS
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 3.3**

  - [x] 8.4 Write property test for inversion self-inverse
    - **Property 4: Inversion is self-inverse**
    - Test that invertValue(invertValue(v)) == v for all v in [0, 127]
    - Test that invertValue(v) == 127 - v
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 3.5**

  - [x] 8.5 Write property test for ModuleDescriptor serialization round-trip
    - **Property 5: ModuleDescriptor serialization round-trip**
    - Test that serialize then deserialize recovers original types, labels, and values
    - Generate random controls (up to 16, labels ≤ 12 chars, values in [0, 127])
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 4.2, 4.4, 4.6**

  - [x] 8.6 Write property test for CMD_READ_VALUES identity
    - **Property 6: CMD_READ_VALUES response is identity over buffer**
    - Test that response[i] == buffer[i] for all i in [0, N)
    - Generate random buffers of N values
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 4.3**

  - [x] 8.7 Write property test for encoder bounded accumulation
    - **Property 7: Encoder accumulation is bounded**
    - Test that encoder value stays in [0, 127] for any sequence of CW/CCW transitions
    - Test CW from any value v produces min(v+1, 127), CCW produces max(v-1, 0)
    - Use RapidCheck with 100+ iterations
    - **Validates: Requirements 7.2, 7.3**

- [ ] 9. Unit tests for edge cases
  - [x] 9.1 Write unit tests for ADC mapping edge cases
    - Test ADC 0 → MIDI 0, ADC 4095 → MIDI 127
    - Test midpoint ADC 2048 → expected MIDI value
    - _Requirements: 3.4_

  - [x] 9.2 Write unit tests for encoder edge cases
    - Test encoder initializes at 64 (MIDI_MID)
    - Test encoder at 127 + CW remains 127 (no overflow)
    - Test encoder at 0 + CCW remains 0 (no underflow)
    - Test invalid transitions (e.g., 00→11) produce no change
    - _Requirements: 7.2, 7.3, 7.4_

  - [x] 9.3 Write unit tests for I2C command handling
    - Test unknown command does not alter state
    - Test CMD_DESCRIPTOR response starts with numControles byte
    - Test CMD_READ_VALUES response length equals NUM_CONTROLES
    - _Requirements: 4.3, 4.5, 4.6_

  - [x] 9.4 Write unit tests for buffer initialization
    - Test valueBuffer initialized with all zeros
    - Test NUM_CONTROLES matches sizeof(CONTROLES)/sizeof(CONTROLES[0])
    - _Requirements: 2.3, 6.4_

- [x] 10. Final checkpoint - All tests pass
  - Run `pio test -e native` to execute all property-based and unit tests, ensure all pass. Ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples and edge cases
- Pure functions (mapAdcToMidi, applyDeadzone, applyDebounce, invertValue, serializeDescriptor, processEncoderTransition) are separated from hardware access to enable host-based testing
- All tests run in PlatformIO native environment (host) using RapidCheck for PBT and Unity for unit tests
