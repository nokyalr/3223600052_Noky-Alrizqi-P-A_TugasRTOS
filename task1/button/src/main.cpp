// ESP32-S3 + Arduino (FreeRTOS)
// Contoh: Button saja, dengan ISR -> Queue -> Task (debounce di task)
//
// - BUTTON_PIN memakai internal pull-up (aktif LOW).
// - ISR kirim event "pressed/released" via xQueueSendFromISR.
// - Task di-pin ke core tertentu, melakukan debouncing & logging.
//
// Ubah BUTTON_PIN sesuai hardware Anda.

#include <Arduino.h>

// =================== Konfigurasi ===================
constexpr uint8_t BUTTON_PIN   = 4;      
constexpr uint32_t DEBOUNCE_MS = 50;     // debouncing di task
constexpr BaseType_t TASK_CORE = 1;      // pin task ke core: 0 atau 1 (ESP32-S3 punya 2 core)

// =================== Tipe & Global =================
struct BtnEvent {
  uint32_t ms;     // waktu (millis) saat interrupt
  bool pressed;    // true = ditekan (LOW), false = dilepas (HIGH)
};

static QueueHandle_t btnQueue = nullptr;

// (opsional) simpan tick ISR terakhir untuk penyaring super cepat (anti-bounce ekstrim di ISR)
static volatile uint32_t s_lastIsrMs = 0;

// =================== ISR ===========================
void IRAM_ATTR buttonIsr() {
  // Baca level sekarang (aktif LOW)
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  uint32_t nowMs = millis();

  // (opsional) hard-limit untuk bounce ekstrem di ISR (mis. 5 ms)
  if ((nowMs - s_lastIsrMs) < 5) return;
  s_lastIsrMs = nowMs;

  BtnEvent ev{nowMs, pressed};
  BaseType_t hpw = pdFALSE;
  if (btnQueue) {
    xQueueSendFromISR(btnQueue, &ev, &hpw);
    if (hpw == pdTRUE) portYIELD_FROM_ISR(); // wake task lebih cepat
  }
}

// =================== Task Handler ==================
void buttonTask(void* pv) {
  (void)pv;
  BtnEvent ev{};
  uint32_t lastHandledMs = 0;
  bool lastStableState = !true; // awalnya "released" (HIGH)

  // Mulai dengan membaca keadaan awal
  lastStableState = (digitalRead(BUTTON_PIN) == LOW); // true jika sedang ditekan saat boot
  Serial.printf("Initial state: %s\n", lastStableState ? "PRESSED" : "RELEASED");

  for (;;) {
    if (xQueueReceive(btnQueue, &ev, portMAX_DELAY) == pdTRUE) {
      // Debounce berbasis waktu di task
      if ((ev.ms - lastHandledMs) < DEBOUNCE_MS) {
        continue; // abaikan bounce
      }
      lastHandledMs = ev.ms;

      // Konfirmasi: baca lagi level saat ini (lebih stabil)
      bool levelNow = (digitalRead(BUTTON_PIN) == LOW);
      if (levelNow != lastStableState) {
        lastStableState = levelNow;
        if (levelNow) {
          // pressed
          Serial.printf("[%lu ms] BUTTON PRESSED\n", (unsigned long)ev.ms);
          // TODO: taruh aksi saat tombol ditekan
        } else {
          // released
          Serial.printf("[%lu ms] BUTTON RELEASED\n", (unsigned long)ev.ms);
          // TODO: taruh aksi saat tombol dilepas
        }
      }
    }
  }
}

// =================== Setup/Loop ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Konfigurasi pin tombol: internal pull-up, aktif LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Buat queue untuk event tombol
  btnQueue = xQueueCreate(10, sizeof(BtnEvent));
  if (!btnQueue) {
    Serial.println("Gagal membuat queue tombol!");
    for (;;) { vTaskDelay(portMAX_DELAY); }
  }

  // Pasang interrupt pada perubahan level (PRESS & RELEASE)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonIsr, CHANGE);

  // Buat task yang dipin ke core pilihan
  BaseType_t ok = xTaskCreatePinnedToCore(
      buttonTask,       // fungsi task
      "ButtonTask",     // nama task
      3072,             // stack size (words)
      nullptr,          // parameter
      3,                // prioritas (sedikit di atas default)
      nullptr,          // handle (tak perlu)
      TASK_CORE         // pin ke core 0 atau 1
  );

  if (ok != pdPASS) {
    Serial.println("Gagal membuat ButtonTask!");
  } else {
    Serial.printf("ButtonTask running on core %d\n", (int)TASK_CORE);
  }
}

void loop() {
  // Kosongkan; RTOS menjalankan task.
  vTaskDelay(portMAX_DELAY);
}
