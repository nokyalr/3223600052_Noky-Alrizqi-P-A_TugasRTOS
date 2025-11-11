#include <Arduino.h>
// ESP32-S3 + Arduino Core + FreeRTOS
// Rotary Encoder KY-040 (A=CLK, B=DT, SW=button)
// ISR ringan -> Queue -> Task (decoder + debounce)

// ---------- Konfigurasi pin (ubah sesuai wiring) ----------
constexpr uint8_t ENC_CLK_PIN = 2;   // Channel A (CLK)
constexpr uint8_t ENC_DT_PIN  = 3;   // Channel B (DT)
constexpr uint8_t ENC_SW_PIN  = 4;   // Switch (push button)

// Pull-up internal aman dipakai walau modul biasanya punya pull-up
constexpr bool USE_INTERNAL_PULLUP = true;

// Task config
constexpr BaseType_t TASK_CORE = 1;    // 0 atau 1
constexpr UBaseType_t TASK_PRIO = 3;

// (opsional) noise guard super cepat di ISR (μs)
constexpr uint32_t MIN_ISR_GAP_US = 100;  // 100–300 μs cukup untuk KY-040

// ---------- Tipe pesan ----------
enum class EncMsgType : uint8_t { AB_EDGE, SW_EDGE };

struct EncABMsg {
  uint8_t abState;   // bit1=B, bit0=A (current)
  uint32_t tMicros;  // timestamp
};

struct EncSWMsg {
  bool pressed;      // true=pressed (LOW), false=released (HIGH)
  uint32_t tMillis;  // timestamp
};

struct EncMsg {
  EncMsgType type;
  union {
    EncABMsg ab;
    EncSWMsg sw;
  } data;
};

// Queue
static QueueHandle_t encQ = nullptr;

// ---------- Variabel ISR ----------
static volatile uint8_t s_lastAB = 0;
static volatile uint32_t s_lastEdgeUs = 0;
static volatile uint32_t s_lastSwMs = 0;

// ---------- Helper baca AB sebagai 2 bit ----------
static inline uint8_t readAB() {
  uint8_t a = (uint8_t)digitalRead(ENC_CLK_PIN);
  uint8_t b = (uint8_t)digitalRead(ENC_DT_PIN);
  return ((b & 1) << 1) | (a & 1);
}

// ---------- ISR untuk kanal A & B ----------
void IRAM_ATTR encABIsr() {
  uint32_t nowUs = micros();
  if (nowUs - s_lastEdgeUs < MIN_ISR_GAP_US) return;  // hard filter
  s_lastEdgeUs = nowUs;

  uint8_t ab = readAB();

  EncMsg m{};
  m.type = EncMsgType::AB_EDGE;
  m.data.ab = EncABMsg{ab, nowUs};

  BaseType_t hpw = pdFALSE;
  if (encQ) xQueueSendFromISR(encQ, &m, &hpw);
  if (hpw == pdTRUE) portYIELD_FROM_ISR();
}

// ---------- ISR tombol ----------
void IRAM_ATTR encSwIsr() {
  uint32_t nowMs = millis();
  // (opsional) 5 ms noise guard
  if (nowMs - s_lastSwMs < 5) return;
  s_lastSwMs = nowMs;

  bool pressed = (digitalRead(ENC_SW_PIN) == LOW);

  EncMsg m{};
  m.type = EncMsgType::SW_EDGE;
  m.data.sw = EncSWMsg{pressed, nowMs};

  BaseType_t hpw = pdFALSE;
  if (encQ) xQueueSendFromISR(encQ, &m, &hpw);
  if (hpw == pdTRUE) portYIELD_FROM_ISR();
}

// ---------- Task decoder ----------
// Tabel transisi Gray 2-bit (prev<<2 | curr):
// +1 / -1 per setengah langkah (bisa kalikan 0.5 jika mau full-step)
static const int8_t TRANSITION_TAB[16] = {
/* 0000 */  0, /*0001*/ +1, /*0010*/ -1, /*0011*/  0,
/* 0100 */ -1, /*0101*/  0, /*0110*/  0, /*0111*/ +1,
/* 1000 */ +1, /*1001*/  0, /*1010*/  0, /*1011*/ -1,
/* 1100 */  0, /*1101*/ -1, /*1110*/ +1, /*1111*/  0
};

struct EncodedState {
  uint8_t prevAB = 0;
  int32_t position = 0;   // satuan "tick" (half-step). Untuk per-step, bagi 2.
  bool swStable = false;  // stable state tombol (pressed=false=HIGH awal)
  uint32_t swLastHandledMs = 0;
};

void encoderTask(void* pv) {
  (void)pv;

  // Inisialisasi state
  EncodedState st{};
  st.prevAB = readAB();
  st.swStable = (digitalRead(ENC_SW_PIN) == LOW); // true jika ditekan saat boot

  EncMsg msg{};
  for (;;) {
    if (xQueueReceive(encQ, &msg, portMAX_DELAY) == pdTRUE) {
      if (msg.type == EncMsgType::AB_EDGE) {
        uint8_t curr = msg.data.ab.abState;
        uint8_t idx = ((st.prevAB & 0x3) << 2) | (curr & 0x3);
        int8_t step = TRANSITION_TAB[idx];

        if (step != 0) {
          st.position += step;
          st.prevAB = curr;

          // Contoh output (half-step). Untuk full-step: pos/2.
          Serial.printf("[enc] pos=%ld (tick)  step=%d  ab=%u%u\n",
                        (long)st.position, step,
                        (curr >> 1) & 1, curr & 1);
          // TODO: kirim ke komponen lain via queue/event group kalau perlu
        } else {
          st.prevAB = curr; // tetap update
        }

      } else if (msg.type == EncMsgType::SW_EDGE) {
        // Debounce tombol di task (mis. 30 ms)
        const uint32_t DEBOUNCE_MS = 30;
        if (msg.data.sw.tMillis - st.swLastHandledMs < DEBOUNCE_MS) continue;
        st.swLastHandledMs = msg.data.sw.tMillis;

        bool levelNowPressed = msg.data.sw.pressed;
        if (levelNowPressed != st.swStable) {
          st.swStable = levelNowPressed;
          if (levelNowPressed) {
            Serial.println("[enc] SW PRESSED");
            // TODO: aksi short-press/long-press bisa ditambah di sini
          } else {
            Serial.println("[enc] SW RELEASED");
          }
        }
      }
    }
  }
}

// ---------- Setup/Loop ----------
void setup() {
  Serial.begin(115200);
  delay(100);

  // Pin mode
  if (USE_INTERNAL_PULLUP) {
    pinMode(ENC_CLK_PIN, INPUT_PULLUP);
    pinMode(ENC_DT_PIN,  INPUT_PULLUP);
    pinMode(ENC_SW_PIN,  INPUT_PULLUP); // aktif LOW
  } else {
    pinMode(ENC_CLK_PIN, INPUT);
    pinMode(ENC_DT_PIN,  INPUT);
    pinMode(ENC_SW_PIN,  INPUT);
  }

  // Buat queue
  encQ = xQueueCreate(32, sizeof(EncMsg));
  if (!encQ) {
    Serial.println("Gagal membuat queue encoder!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  // Pasang interrupt
  attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encABIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT_PIN),  encABIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_SW_PIN),  encSwIsr, CHANGE);

  // Task decoder
  BaseType_t ok = xTaskCreatePinnedToCore(
      encoderTask, "EncoderTask",
      4096, nullptr, TASK_PRIO, nullptr, TASK_CORE
  );
  if (ok != pdPASS) Serial.println("Gagal membuat EncoderTask!");

  Serial.println("Encoder siap. Putar knob & tekan tombol.");
}

void loop() {
  // Di loop, Anda bisa konsumsi posisi global, atau biarkan kosong
  vTaskDelay(pdMS_TO_TICKS(1000));
}
