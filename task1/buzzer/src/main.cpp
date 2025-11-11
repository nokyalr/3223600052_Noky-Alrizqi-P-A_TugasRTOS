#include <Arduino.h>

// =================== Konfigurasi ===================
constexpr uint8_t  BUZZER_PIN   = 1;      // ganti sesuai pin buzzer
constexpr uint8_t  LEDC_CH      = 0;      // channel LEDC 0..7
constexpr uint8_t  LEDC_TIMER   = 0;      // timer LEDC 0..3
constexpr uint8_t  LEDC_RES_BITS= 10;     // resolusi PWM
constexpr BaseType_t TASK_CORE  = 1;      // pin task ke core 0/1

// =================== Tipe & Global =================
struct BeepCmd {
  uint16_t freqHz;       // 0 = diam (pause)
  uint16_t durMs;        // durasi beep/pause
  uint8_t  dutyPct;      // 0..100 (umumnya 50 untuk passive buzzer)
};

static QueueHandle_t buzzerQ = nullptr;

// =================== Util ==========================
inline void enqueueBeep(uint16_t freq, uint16_t durMs, uint8_t dutyPct = 50) {
  if (!buzzerQ) return;
  BeepCmd cmd{freq, durMs, dutyPct};
  xQueueSend(buzzerQ, &cmd, 0);
}

inline void enqueuePause(uint16_t durMs) {
  enqueueBeep(0, durMs, 0);
}

// Contoh: pola “double-beep”
void enqueueDoubleBeep() {
  enqueueBeep(1000, 120, 50);
  enqueuePause(60);
  enqueueBeep(1500, 140, 50);
}

// Contoh: nada sederhana (C5–E5–G5–C6)
void enqueueSimpleMelody() {
  enqueueBeep(523, 160);  enqueuePause(40);
  enqueueBeep(659, 160);  enqueuePause(40);
  enqueueBeep(784, 160);  enqueuePause(60);
  enqueueBeep(1046, 240);
}

// =================== Task ==========================
void buzzerTask(void* pv) {
  (void)pv;

  // Siapkan LEDC
  ledcSetup(LEDC_CH, 1000 /*dummy*/, LEDC_RES_BITS);
  ledcAttachPin(BUZZER_PIN, LEDC_CH);

  BeepCmd cmd{};
  for (;;) {
    if (xQueueReceive(buzzerQ, &cmd, portMAX_DELAY) == pdTRUE) {
      if (cmd.freqHz == 0 || cmd.dutyPct == 0) {
        // Pause / diam
        ledcWrite(LEDC_CH, 0);
        vTaskDelay(pdMS_TO_TICKS(cmd.durMs));
      } else {
        // Hitung duty dari persen
        uint32_t maxDuty = (1u << LEDC_RES_BITS) - 1u;
        uint32_t duty = (uint32_t)(maxDuty * cmd.dutyPct / 100u);

        // Set frekuensi & duty
        ledcWriteTone(LEDC_CH, cmd.freqHz); // set freq
        ledcWrite(LEDC_CH, duty);           // set duty

        vTaskDelay(pdMS_TO_TICKS(cmd.durMs));

        // Matikan setelah durasi (supaya "putus" antar nada)
        ledcWrite(LEDC_CH, 0);
      }
    }
  }
}

// =================== Setup/Loop ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Queue untuk perintah buzzer
  buzzerQ = xQueueCreate(16, sizeof(BeepCmd));
  if (!buzzerQ) {
    Serial.println("Gagal membuat queue buzzer!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  // Buat task buzzer
  BaseType_t ok = xTaskCreatePinnedToCore(
    buzzerTask,
    "BuzzerTask",
    3072,        // stack words
    nullptr,
    3,           // prioritas
    nullptr,
    TASK_CORE
  );
  if (ok != pdPASS) Serial.println("Gagal membuat BuzzerTask!");

  // --- Demo: kirim beberapa pola saat start ---
  enqueueDoubleBeep();
  enqueuePause(200);
  enqueueSimpleMelody();
}

void loop() {
  // Tidak dipakai; semua lewat RTOS task.
  vTaskDelay(portMAX_DELAY);
}
