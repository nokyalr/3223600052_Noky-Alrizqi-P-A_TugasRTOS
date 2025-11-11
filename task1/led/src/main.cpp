// ESP32-S3 + Arduino Core + FreeRTOS
// 3 LED pada GPIO 5, 6, 7
// Tiap LED dikendalikan task RTOS yang dipin ke core tertentu.

#include <Arduino.h>

// --- Konfigurasi pin LED ---
constexpr uint8_t LED1_PIN = 5;
constexpr uint8_t LED2_PIN = 6;
constexpr uint8_t LED3_PIN = 7;

// --- Konfigurasi interval blink (ms) ---
constexpr uint32_t LED1_PERIOD = 200;   // cepat
constexpr uint32_t LED2_PERIOD = 500;   // sedang
constexpr uint32_t LED3_PERIOD = 1000;  // lambat

// Struktur parameter untuk task
struct LedTaskConfig {
  uint8_t pin;
  TickType_t periodTicks;
};

// Parameter statis supaya alamatnya tetap valid selama runtime
static LedTaskConfig led1Cfg{LED1_PIN, pdMS_TO_TICKS(LED1_PERIOD)};
static LedTaskConfig led2Cfg{LED2_PIN, pdMS_TO_TICKS(LED2_PERIOD)};
static LedTaskConfig led3Cfg{LED3_PIN, pdMS_TO_TICKS(LED3_PERIOD)};

// Handle task (opsional, kalau mau digunakan kemudian)
TaskHandle_t hTaskLed1 = nullptr;
TaskHandle_t hTaskLed2 = nullptr;
TaskHandle_t hTaskLed3 = nullptr;

// Fungsi task generik untuk kedip LED
void ledTask(void *pv) {
  auto *cfg = reinterpret_cast<LedTaskConfig *>(pv);
  const uint8_t pin = cfg->pin;
  const TickType_t period = cfg->periodTicks;

  // Pastikan pin sebagai output dan mulai LOW
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);

  // Loop RTOS
  TickType_t lastWake = xTaskGetTickCount();
  bool state = false;
  for (;;) {
    state = !state;
    digitalWrite(pin, state ? HIGH : LOW);

    // Delay periodik, presisi RTOS
    vTaskDelayUntil(&lastWake, period);
  }
}

void setup() {
  // (opsional) serial debug
  Serial.begin(115200);
  delay(100);

  // Inisialisasi pin (aman meskipun juga diatur di task)
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  // --- Buat task dan pin ke core tertentu ---
  // ESP32-S3 memiliki 2 core: 0 dan 1.
  // Distribusi contoh: dua task di core 0, satu task di core 1.

  BaseType_t ok;

  ok = xTaskCreatePinnedToCore(
        ledTask,            // fungsi task
        "LED1_Task",        // nama
        2048,               // stack size (words)
        &led1Cfg,           // parameter
        2,                  // prioritas (lebih tinggi = lebih prioritas)
        &hTaskLed1,         // handle
        0                   // core 0
      );
  if (ok != pdPASS) Serial.println("Gagal membuat LED1_Task");

  ok = xTaskCreatePinnedToCore(
        ledTask,
        "LED2_Task",
        2048,
        &led2Cfg,
        2,
        &hTaskLed2,
        1                   // core 1
      );
  if (ok != pdPASS) Serial.println("Gagal membuat LED2_Task");

  ok = xTaskCreatePinnedToCore(
        ledTask,
        "LED3_Task",
        2048,
        &led3Cfg,
        2,
        &hTaskLed3,
        0                   // core 0
      );
  if (ok != pdPASS) Serial.println("Gagal membuat LED3_Task");

  Serial.println("Tasks LED berjalan di dua core.");
}

void loop() {
  // Tidak digunakan. Biarkan RTOS menjalankan tasks.
  vTaskDelay(portMAX_DELAY);
}
