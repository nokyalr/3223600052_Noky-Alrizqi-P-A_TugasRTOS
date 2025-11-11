// ESP32-S3 — MULTI TASK DEMO (RTOS)
// Modul: Stepper A4988, 3 LED, Button (ISR+queue), Buzzer LEDC,
//        OLED SSD1306 (I2C), Potentiometer (ADC), Servo (LEDC 50Hz),
//        Rotary Encoder (A/B ISR + SW).
// Tiap modul berjalan mandiri dalam task terpisah.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -------------------- PIN MAP (ubah jika perlu) --------------------
#define LED1 5
#define LED2 6
#define LED3 7

#define BTN_PIN 4              // tombol terpisah dari encoder SW
#define BUZZ_PIN 8
#define SERVO_PIN 9
#define I2C_SDA 10
#define I2C_SCL 11
#define POT_PIN 1

#define ENC_A 2
#define ENC_B 3
#define ENC_SW 15              // pakai 15 agar tidak bentrok BTN_PIN

#define STEP_PIN 18
#define DIR_PIN  19

// -------------------- GLOBAL / OLED --------------------
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// =================================================================
// LED TASK
void LedTask(void *) {
  pinMode(LED1, OUTPUT); pinMode(LED2, OUTPUT); pinMode(LED3, OUTPUT);
  for (;;) {
    digitalWrite(LED1, !digitalRead(LED1)); vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(LED2, !digitalRead(LED2)); vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(LED3, !digitalRead(LED3)); vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// =================================================================
// BUTTON (ISR → Queue → Task)
struct BtnEv { bool pressed; uint32_t ms; };
QueueHandle_t btnQ;
void IRAM_ATTR btnIsr() {
  BtnEv e{digitalRead(BTN_PIN)==LOW, millis()};
  BaseType_t hp=pdFALSE; xQueueSendFromISR(btnQ,&e,&hp); if(hp) portYIELD_FROM_ISR();
}
void ButtonTask(void *) {
  pinMode(BTN_PIN, INPUT_PULLUP);
  for (BtnEv e;;) if (xQueueReceive(btnQ,&e,portMAX_DELAY)==pdTRUE)
    Serial.println(e.pressed ? "[BTN] PRESSED" : "[BTN] RELEASED");
}

// =================================================================
// BUZZER (LEDC)
#define BZ_CH 0
void BuzzerTask(void *) {
  ledcSetup(BZ_CH, 1000, 10);
  ledcAttachPin(BUZZ_PIN, BZ_CH);
  for (;;) {
    ledcWriteTone(BZ_CH, 1000); ledcWrite(BZ_CH, 512); vTaskDelay(pdMS_TO_TICKS(120));
    ledcWrite(BZ_CH, 0);        vTaskDelay(pdMS_TO_TICKS(60));
    ledcWriteTone(BZ_CH, 1500); ledcWrite(BZ_CH, 512); vTaskDelay(pdMS_TO_TICKS(140));
    ledcWrite(BZ_CH, 0);        vTaskDelay(pdMS_TO_TICKS(800));
  }
}

// =================================================================
// OLED SSD1306 (I2C)
void OledTask(void *) {
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init FAIL");
    for(;;) vTaskDelay(pdMS_TO_TICKS(1000));
  }
  for (;;) {
    oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2); oled.setCursor(0,0);   oled.print("ESP32-S3");
    oled.setTextSize(1); oled.setCursor(0,26);  oled.print("Multi Task Demo");
    oled.setCursor(0,40); oled.printf("Uptime: %lus", (unsigned long)(millis()/1000));
    oled.display();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// =================================================================
// POTENTIOMETER (ADC)
void PotTask(void *) {
  analogReadResolution(12); analogSetPinAttenuation(POT_PIN, ADC_11db);
  for (;;) {
    uint16_t raw = analogRead(POT_PIN);
    uint8_t pct = map(raw, 0, 4095, 0, 100);
    Serial.printf("[POT] raw=%u  %u%%\n", raw, pct);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// =================================================================
// SERVO (LEDC 50 Hz)
#define SV_CH 2
uint32_t dutyFromUs(uint32_t us) { const uint32_t max=(1<<14)-1; return (uint32_t)((uint64_t)us*max/20000); }
void ServoTask(void *) {
  ledcSetup(SV_CH, 50, 14); ledcAttachPin(SERVO_PIN, SV_CH);
  auto writeUs = [&](uint32_t us){ ledcWrite(SV_CH, dutyFromUs(us)); };
  for(;;){
    writeUs(500);  vTaskDelay(pdMS_TO_TICKS(700));
    writeUs(1500); vTaskDelay(pdMS_TO_TICKS(700));
    writeUs(2500); vTaskDelay(pdMS_TO_TICKS(700));
  }
}

// =================================================================
// ROTARY ENCODER (A/B ISR + SW ISR)
volatile int32_t encPos=0; volatile uint8_t lastAB=0;
uint8_t rdAB(){ return ((uint8_t)digitalRead(ENC_B)<<1) | digitalRead(ENC_A); }
void IRAM_ATTR encABisr(){
  uint8_t ab=rdAB(); uint8_t idx=((lastAB&3)<<2)|(ab&3);
  static const int8_t T[16]={0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
  encPos+=T[idx]; lastAB=ab;
}
void IRAM_ATTR encSWisr(){ if (digitalRead(ENC_SW)==LOW) Serial.println("[ENC] SW"); }
void EncTask(void *) {
  pinMode(ENC_A,INPUT_PULLUP); pinMode(ENC_B,INPUT_PULLUP); pinMode(ENC_SW,INPUT_PULLUP);
  lastAB=rdAB();
  attachInterrupt(digitalPinToInterrupt(ENC_A), encABisr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encABisr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_SW), encSWisr, FALLING);
  for(;;){ Serial.printf("[ENC] pos=%ld\n",(long)encPos); vTaskDelay(pdMS_TO_TICKS(300)); }
}

// =================================================================
// STEPPER A4988 (tanpa EN, pola maju→pause→mundur, ala contoh)
const int stepsPerRev = 200;   // ubah sesuai microstep
volatile int stepDelayUS = 1000; // step high & low (1k us) → ~500 step/s
void StepperTask(void *) {
  pinMode(STEP_PIN, OUTPUT); pinMode(DIR_PIN, OUTPUT);
  for (;;) {
    // CW
    digitalWrite(DIR_PIN, HIGH);
    for (int i=0;i<stepsPerRev;i++){ digitalWrite(STEP_PIN,HIGH); ets_delay_us(stepDelayUS); digitalWrite(STEP_PIN,LOW); ets_delay_us(stepDelayUS); }
    vTaskDelay(pdMS_TO_TICKS(1000));
    // CCW
    digitalWrite(DIR_PIN, LOW);
    for (int i=0;i<stepsPerRev;i++){ digitalWrite(STEP_PIN,HIGH); ets_delay_us(stepDelayUS); digitalWrite(STEP_PIN,LOW); ets_delay_us(stepDelayUS); }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// =================================================================
// SETUP & LOOP
void setup() {
  Serial.begin(115200);
  // Queue untuk tombol
  btnQ = xQueueCreate(8, sizeof(BtnEv));
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), btnIsr, CHANGE);

  // Buat semua task (boleh ganti core 0/1 sesuka)
  xTaskCreatePinnedToCore(LedTask,     "LED",     2048, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(ButtonTask,  "BTN",     2048, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(BuzzerTask,  "BUZZ",    2048, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(OledTask,    "OLED",    4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(PotTask,     "POT",     2048, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(ServoTask,   "SERVO",   2048, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(EncTask,     "ENC",     3072, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(StepperTask, "STEPPER", 2048, nullptr, 2, nullptr, 0);
}

void loop() { /* kosong, semua lewat task */ }
