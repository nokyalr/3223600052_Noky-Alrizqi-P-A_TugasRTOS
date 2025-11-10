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

## Pemetaan pin (diagram.json)
<img width="782" height="418" alt="image" src="https://github.com/user-attachments/assets/251c92ad-b1cf-4278-8113-a540d1f0a94f" />

- LED1: GPIO10
- LED2: GPIO11
- LED3: GPIO12
- BUTTON1: GPIO13 (INPUT_PULLUP)
- BUTTON2: GPIO14 (INPUT_PULLUP)
- BUZZER (PWM): GPIO3
- OLED (I2C): SDA = GPIO8, SCL = GPIO9
- POT (ADC): GPIO1 (ADC1 channel)
- ENCODER CLK (A): GPIO6
- ENCODER DT (B): GPIO7
- ENCODER SW: GPIO5
- STEPPER IN1 (A-): GPIO21
- STEPPER IN2 (A+): GPIO39
- STEPPER IN3 (B+): GPIO40
- STEPPER IN3 (B-): GPIO38
- SERVO (PWM): GPIO16

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
https://github.com/user-attachments/assets/b5219cc6-3a79-4fd0-934b-dc024594a8c2

