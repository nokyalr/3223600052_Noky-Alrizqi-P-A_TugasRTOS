// ESP32-S3 + Arduino + FreeRTOS
// OLED SSD1306 I2C (128x64) dengan task & queue untuk update tampilan

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =================== Konfigurasi ===================
// Atur SDA/SCL sesuai board Anda:
constexpr int I2C_SDA_PIN   = 10;      // ganti jika perlu
constexpr int I2C_SCL_PIN   = 11;      // ganti jika perlu
constexpr uint32_t I2C_FREQ = 400000;  // 400 kHz (bisa 100 kHz jika modul sensitif)

constexpr uint8_t OLED_ADDR = 0x3C;    // alamat umum SSD1306
constexpr int OLED_W = 128;
constexpr int OLED_H = 64;

constexpr BaseType_t OLED_TASK_CORE = 1;  // pin task ke core 0/1
constexpr UBaseType_t OLED_TASK_PRIO = 3;
constexpr uint16_t    OLED_TASK_STACK = 4096; // words

// =================== Tipe & Global =================
struct OledMsg {
  // Pesan sederhana: 4 baris max, ukuran teks 1..3
  char line1[22];
  char line2[22];
  char line3[22];
  char line4[22];
  uint8_t textSize;     // skala font (1..3)
  bool invert;          // true = inverse color
};

// Queue untuk pesan layar
static QueueHandle_t oledQ = nullptr;

// Driver OLED (buffered)
static Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1 /*no reset pin*/);

// =================== Util: kirim pesan ke OLED =====
void oledPrint(const char* l1 = "", const char* l2 = "", const char* l3 = "", const char* l4 = "", uint8_t size = 1, bool inv = false) {
  if (!oledQ) return;
  OledMsg m{};
  strncpy(m.line1, l1, sizeof(m.line1) - 1);
  strncpy(m.line2, l2, sizeof(m.line2) - 1);
  strncpy(m.line3, l3, sizeof(m.line3) - 1);
  strncpy(m.line4, l4, sizeof(m.line4) - 1);
  m.textSize = constrain(size, 1, 3);
  m.invert = inv;
  xQueueSend(oledQ, &m, 0);
}

// =================== Task OLED =====================
void oledTask(void* pv) {
  (void)pv;

  // Init I2C & display
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR, /*reset=*/false, /*periphBegin=*/false)) {
    Serial.println("SSD1306 init FAILED!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("SSD1306 Ready"));
  display.display();

  OledMsg msg{};
  TickType_t lastBlink = xTaskGetTickCount();
  bool heartbeat = false;

  for (;;) {
    // Tampilkan pesan jika ada
    if (xQueueReceive(oledQ, &msg, pdMS_TO_TICKS(50)) == pdTRUE) {
      display.clearDisplay();

      if (msg.invert) display.invertDisplay(true);
      else            display.invertDisplay(false);

      display.setTextSize(msg.textSize);
      display.setCursor(0, 0);
      display.println(msg.line1);
      display.println(msg.line2);
      display.println(msg.line3);
      display.println(msg.line4);

      // Footer kecil: heartbeat (indikator hidup)
      display.setTextSize(1);
      display.setCursor(0, OLED_H - 8);
      display.print("HB:");
      display.print(heartbeat ? "1 " : "0 ");
      display.display();
    }

    // Heartbeat berkedip tiap 500 ms di pojok kanan bawah
    if (xTaskGetTickCount() - lastBlink >= pdMS_TO_TICKS(500)) {
      lastBlink = xTaskGetTickCount();
      heartbeat = !heartbeat;

      // gambar titik kecil tanpa mengganggu konten utama
      // (update area kecil saja)
      display.fillRect(OLED_W - 10, OLED_H - 10, 10, 10, SSD1306_BLACK);
      if (heartbeat) display.fillCircle(OLED_W - 5, OLED_H - 5, 3, SSD1306_WHITE);
      display.display();
    }
  }
}

// =================== Setup/Loop ====================
void setup() {
  Serial.begin(115200);
  delay(100);

  // Buat queue
  oledQ = xQueueCreate(8, sizeof(OledMsg));
  if (!oledQ) {
    Serial.println("Gagal membuat queue OLED!");
    for (;;) vTaskDelay(portMAX_DELAY);
  }

  // Buat task OLED
  BaseType_t ok = xTaskCreatePinnedToCore(
      oledTask,
      "OLED_Task",
      OLED_TASK_STACK,
      nullptr,
      OLED_TASK_PRIO,
      nullptr,
      OLED_TASK_CORE
  );
  if (ok != pdPASS) {
    Serial.println("Gagal membuat OLED_Task!");
  }

  // ===== Demo pesan =====
  oledPrint("ESP32-S3", "SSD1306 (I2C)", "FreeRTOS + Queue", "Siap!", 1, false);
  vTaskDelay(pdMS_TO_TICKS(1200));
  oledPrint("Line1 besar", "Line2", "Line3", "Line4", 2, false);
  vTaskDelay(pdMS_TO_TICKS(1200));
  oledPrint("Inverted", "contoh", "", "", 2, true);
}

void loop() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs > 3000) {
    lastMs = now;
    char buf[22];
    snprintf(buf, sizeof(buf), "Uptime: %lus", now / 1000UL);
    oledPrint("Status OK", buf, "", "", 1, false);
  }

  vTaskDelay(pdMS_TO_TICKS(50));
}
