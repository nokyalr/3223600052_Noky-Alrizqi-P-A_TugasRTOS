## ESP32-S3 RTOS — Percobaan Peripheral (2 Core) -- Wokwi Simulator

Dokumentasi singkat untuk menghubungkan dan menjalankan peripheral pada ESP32-S3 (2 core) menggunakan FreeRTOS.

Perangkat yang digunakan:
- 3 × LED
- 2 × Push button
- 1 × Buzzer (active/passive)
- 1 × OLED (I2C, 128x64, addr 0x3C)
- 1 × Potensiometer (ADC)
- 1 × Rotary encoder (A, B, push)
- 1 × Motor stepper (dengan driver step/dir)
- 1 × Servo (PWM)

## Pemetaan pin (diagram.Json)
- LED1: GPIO2
- LED2: GPIO4
- LED3: GPIO5
- BUTTON1: GPIO0 (INPUT_PULLUP)
- BUTTON2: GPIO15 (INPUT_PULLUP)
- BUZZER (PWM): GPIO13
- OLED (I2C): SDA = GPIO21, SCL = GPIO22
- POT (ADC): GPIO36 (ADC1 channel)
- ENCODER A: GPIO18
- ENCODER B: GPIO19
- ENCODER BTN: GPIO23
- STEPPER STEP: GPIO26
- STEPPER DIR: GPIO25
- STEPPER ENABLE: GPIO27
- SERVO (PWM): GPIO14

## Program (main.cpp)
File program terdapat pada folder src -> main.cpp

## Cara menjalankan tiap I/O di tiap core (FreeRTOS)
Pada ESP32 dapat membuat task FreeRTOS dan mem-pinnya ke core tertentu dengan `xTaskCreatePinnedToCore`. Contoh singkat pembuatan 2 tugas: satu di core 0 dan satu di core 1.

Contoh (singkat) — potongan kode di `src/main.cpp`:

```cpp
// Contoh pembuatan task pinned ke core
void taskLEDs(void *param) {
  while (true) {
    // toggling LED
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void taskButtonsBuzzer(void *param) {
  while (true) {
    // cek button
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  // inisialisasi pin
  xTaskCreatePinnedToCore(taskLEDs,        "LEDs",        4096, NULL, 1, &taskLEDsHandle,       0);
  xTaskCreatePinnedToCore(taskServoOLED,   "ServoOLED",   6144, NULL, 1, &taskServoOLEDHandle,   1);  // core 1
}

void loop() {
  // kosong, semua kerja di task
}
```

## Hasil Video Demo
