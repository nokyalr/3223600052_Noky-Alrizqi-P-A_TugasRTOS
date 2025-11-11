// ESP32-S3 + Arduino Core + FreeRTOS
// Servo via LEDC 50 Hz + Task + Queue (smooth move)

#include <Arduino.h>

// =================== Konfigurasi ===================
constexpr int SERVO_PIN        = 9;   // pin PWM untuk sinyal servo
constexpr uint8_t LEDC_CH      = 2;   // channel LEDC (0..7)
constexpr uint8_t LEDC_TIMER   = 1;   // timer LEDC (0..3)
constexpr uint32_t SERVO_FREQ  = 50;  // 50 Hz (period 20 ms)
constexpr uint8_t LEDC_RES_BITS= 14;  // resolusi PWM (14 bit cukup halus)

constexpr int MIN_PULSE_US     = 500;  // batas bawah (us) ~0°
constexpr int MAX_PULSE_US     = 2500; // batas atas (us) ~180°
constexpr int MIN_ANGLE        = 0;    // derajat
constexpr int MAX_ANGLE        = 180;  // derajat

// Task config
constexpr BaseType_t TASK_CORE = 1;    // pin ke core 0/1
constexpr UBaseType_t TASK_PRIO= 3;

// =================== Tipe & Global =================
struct ServoCmd {
  int targetDeg;   // 0..180
  uint8_t speed;   // derajat per langkah (0 = lompat langsung)
  uint16_t stepMs; // interval antar langkah (ms), default 20..30 ms
};

static QueueHandle_t servoQ = nullptr;

// =================== Util ==========================
// Konversi derajat -> duty LEDC
uint32_t angleToDuty(int deg) {
  deg = constrain(deg, MIN_ANGLE, MAX_ANGLE);
  const uint32_t periodUs = 1000000UL / SERVO_FREQ;  // 20_000 us
  const uint32_t pulseUs = map(deg, MIN_ANGLE, MAX_ANGLE, MIN_PULSE_US, MAX_PULSE_US);
  const uint32_t maxDuty = (1UL << LEDC_RES_BITS) - 1UL;
  // duty = (pulse / period) * max
  return (uint32_t)((uint64_t)pulseUs * maxDuty / periodUs);
}

inline void servoWriteAngle(int deg) {
  ledcWrite(LEDC_CH, angleToDuty(deg));
}

// API kirim perintah
void servoMoveTo(int deg, uint8_t speed = 5, uint16_t stepMs = 20) {
  if (!servoQ) return;
  ServoCmd c{deg, speed, stepMs};
  xQueueSend(servoQ, &c, 0);
}

// =================== Task ==========================
void servoTask(void* pv) {
  (void)pv;

  // Init LEDC 50 Hz di resolusi yang dipilih
  ledcSetup(LEDC_CH, SERVO_FREQ, LEDC_RES_BITS);
  ledcAttachPin(SERVO_PIN, LEDC_CH);

  // Inisialisasi posisi awal (90°)
  int currentDeg = 90;
  servoWriteAngle(currentDeg);

  ServoCmd cmd{};
  TickType_t lastStep = xTaskGetTickCount();

  for (;;) {
    // Terima command (non-blocking pendek agar bisa smoothing jalan)
    if (xQueueReceive(servoQ, &cmd, pdMS_TO_TICKS(5)) == pdTRUE) {
      cmd.targetDeg = constrain(cmd.targetDeg, MIN_ANGLE, MAX_ANGLE);
      if (cmd.speed == 0) {
        // Lompat langsung
        currentDeg = cmd.targetDeg;
        servoWriteAngle(currentDeg);
      }
      // kalau speed > 0, smoothing dilakukan di loop di bawah
    }

    // Smoothing menuju target (jika ada dan speed > 0)
    if (cmd.speed > 0) {
      if (xTaskGetTickCount() - lastStep >= pdMS_TO_TICKS(cmd.stepMs)) {
        lastStep = xTaskGetTickCount();
        if (currentDeg < cmd.targetDeg) {
          currentDeg = min(currentDeg + cmd.speed, cmd.targetDeg);
          servoWriteAngle(currentDeg);
        } else if (currentDeg > cmd.targetDeg) {
          currentDeg = max(currentDeg - cmd.speed, cmd.targetDeg);
          servoWriteAngle(currentDeg);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// =================== Setup/Loop ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Queue perintah servo
  servoQ = xQueueCreate(10, sizeof(ServoCmd));
  if (!servoQ) {
    Serial.println("Gagal membuat queue servo!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  // Buat task servo
  BaseType_t ok = xTaskCreatePinnedToCore(
    servoTask, "ServoTask",
    3072, nullptr, TASK_PRIO, nullptr, TASK_CORE
  );
  if (ok != pdPASS) Serial.println("Gagal membuat ServoTask!");

  // ===== Demo gerakan =====
  vTaskDelay(pdMS_TO_TICKS(500));
  servoMoveTo(0,   6, 20);   // ke 0° cukup cepat
  vTaskDelay(pdMS_TO_TICKS(1200));
  servoMoveTo(180, 3, 20);   // ke 180° lebih halus
  vTaskDelay(pdMS_TO_TICKS(2000));
  servoMoveTo(90,  5, 15);   // kembali 90°
}

void loop() {
  static uint32_t t0 = 0;
  uint32_t now = millis();
  if (now - t0 > 5000) {
    t0 = now;
    static bool dir = false;
    dir = !dir;
    servoMoveTo(dir ? 150 : 30, 4, 20);
  }

  vTaskDelay(pdMS_TO_TICKS(10));
}
