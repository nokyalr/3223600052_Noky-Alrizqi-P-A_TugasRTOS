#include <Arduino.h>

constexpr int POT_PIN = 1;

// Setelan sampling & filter
constexpr uint32_t SAMPLE_PERIOD_MS = 10;   // interval baca ADC
constexpr int      SMA_TAPS         = 16;   // panjang moving average (semakin besar, semakin halus)

// Publikasi event hanya jika berubah cukup signifikan
constexpr int      PCT_STEP         = 1;    // minimal perubahan persen utk kirim event
constexpr int      MV_STEP          = 20;   // minimal perubahan mV utk kirim event

// ADC config
constexpr int      ADC_RES_BITS     = 12;   // resolusi pembacaan (0..4095)
constexpr auto     ADC_ATTEN        = ADC_11db; // jangkauan ~0..3.3V (lebih luas)
constexpr BaseType_t TASK_CORE      = 1;    // pin task ke core 0/1

// =================== Tipe & Global =================
struct PotReading {
  uint16_t raw;     // 0..(2^ADC_RES_BITS - 1)
  uint16_t mv;      // miliVolt (aproksimasi)
  uint8_t  percent; // 0..100
};

static QueueHandle_t potQ = nullptr;

// =================== Util ==========================
static inline uint8_t rawToPercent(uint16_t raw) {
  uint32_t maxRaw = (1u << ADC_RES_BITS) - 1u;
  uint32_t pct = (uint32_t)raw * 100u / maxRaw;
  if (pct > 100u) pct = 100u;
  return (uint8_t)pct;
}

// =================== Task ADC ======================
void potTask(void* pv) {
  (void)pv;

  // Konfigurasi ADC
  analogReadResolution(ADC_RES_BITS);
  analogSetPinAttenuation(POT_PIN, ADC_ATTEN);
  pinMode(POT_PIN, INPUT);

  // Buffer untuk Simple Moving Average
  uint16_t buf[SMA_TAPS] = {0};
  uint32_t sum = 0;
  int idx = 0;
  bool filled = false;

  // Nilai terakhir yang sudah dipublish
  int lastPct = -1;
  int lastMv  = -10000;

  // Priming: isi buffer awal agar start stabil
  for (int i = 0; i < SMA_TAPS; ++i) {
    uint16_t v = analogRead(POT_PIN);
    buf[i] = v; sum += v;
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  filled = true;

  for (;;) {
    // Baca ADC
    uint16_t sample = analogRead(POT_PIN);

    // Update SMA
    if (filled) {
      sum -= buf[idx];
    }
    buf[idx] = sample;
    sum += sample;
    idx = (idx + 1) % SMA_TAPS;
    if (!filled && idx == 0) filled = true;

    // Hitung nilai rata2
    uint16_t avg = filled ? (uint16_t)(sum / SMA_TAPS) : sample;

    // Dapatkan mV (Arduino-ESP32 menyediakan analogReadMilliVolts utk ADC1)
    // Jika tidak akurat di board Anda, tetap bisa pakai perkiraan dari raw->percent.
    uint16_t mv = analogReadMilliVolts(POT_PIN);

    uint8_t pct = rawToPercent(avg);

    // Publish hanya saat perubahan berarti
    if ( (lastPct < 0) ||
         (abs((int)pct - lastPct) >= PCT_STEP) ||
         (abs((int)mv  - lastMv ) >= MV_STEP) ) {

      PotReading r{avg, mv, pct};
      if (potQ) xQueueSend(potQ, &r, 0);

      lastPct = pct;
      lastMv  = mv;
    }

    vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
  }
}

// =================== Setup/Loop ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Buat queue untuk publish nilai pot
  potQ = xQueueCreate(16, sizeof(PotReading));
  if (!potQ) {
    Serial.println("Gagal membuat queue pot!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  // Buat task pembaca pot
  BaseType_t ok = xTaskCreatePinnedToCore(
      potTask,
      "PotTask",
      3072,          // stack words
      nullptr,
      3,             // prioritas
      nullptr,
      TASK_CORE
  );
  if (ok != pdPASS) {
    Serial.println("Gagal membuat PotTask!");
  }
}

void loop() {

  PotReading r;
  if (xQueueReceive(potQ, &r, pdMS_TO_TICKS(200)) == pdTRUE) {
    Serial.printf("POT raw=%u  mv=%u  %u%%\n", r.raw, r.mv, r.percent);
  }

}