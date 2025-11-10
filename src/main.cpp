#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ================== Pin Mapping ==================
#define LED1_PIN   10
#define LED2_PIN   11
#define LED3_PIN   12

#define BTN1_PIN   13
#define BTN2_PIN   14

#define BUZZER_PIN 3

// OLED I2C
#define I2C_SDA    8
#define I2C_SCL    9
#define OLED_ADDR  0x3C
Adafruit_SSD1306 display(128, 64, &Wire);

// Potentiometer (ADC)
#define POT_PIN    1

// Rotary Encoder (KY-040)
#define ENC_A_PIN  6
#define ENC_B_PIN  7
#define ENC_SW_PIN 5

// Stepper bipolar 4-kawat (A-, A+, B+, B-)
#define ST_A_N     21
#define ST_A_P     39
#define ST_B_P     40
#define ST_B_N     38

// Servo
#define SERVO_PIN  16
Servo servo1;

// ================== Variabel Global / RTOS ==================
volatile long encCount = 0;
volatile uint8_t lastA = 0, lastB = 0;
volatile bool encBtnPressed = false;

TaskHandle_t taskLEDsHandle;
TaskHandle_t taskBtnBuzzHandle;
TaskHandle_t taskServoOLEDHandle;
TaskHandle_t taskStepperHandle;

hw_timer_t* beeperTimer = nullptr;

// ================== Utilitas ==================
void setStepperCoils(uint8_t a_n, uint8_t a_p, uint8_t b_p, uint8_t b_n) {
  digitalWrite(ST_A_N, a_n);
  digitalWrite(ST_A_P, a_p);
  digitalWrite(ST_B_P, b_p);
  digitalWrite(ST_B_N, b_n);
}

// 8-step half-stepping untuk bipolar
const uint8_t STEP_TABLE[8][4] = {
  {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,1,0},
  {0,0,1,0}, {0,0,1,1}, {0,0,0,1}, {1,0,0,1}
};

// Buzzer menggunakan LEDC tone
const int BUZZR_CH = 2;
void beepStart(int freq) {
  ledcAttachPin(BUZZER_PIN, BUZZR_CH);
  ledcWriteTone(BUZZR_CH, freq);
}
void beepStop() {
  ledcWrite(BUZZR_CH, 0);
  ledcDetachPin(BUZZER_PIN);
}

// ================== ISR Encoder ==================
void IRAM_ATTR encISR_A() {
  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  if (a != lastA) {
    // arah berdasarkan B saat A berubah
    (a == b) ? encCount++ : encCount--;
    lastA = a;
    lastB = b;
  }
}
void IRAM_ATTR encISR_B() {
  uint8_t a = digitalRead(ENC_A_PIN);
  uint8_t b = digitalRead(ENC_B_PIN);
  if (b != lastB) {
    (a != b) ? encCount++ : encCount--;
    lastA = a;
    lastB = b;
  }
}
void IRAM_ATTR encBtnISR() {
  // tombol ke GND, aktif LOW
  if (digitalRead(ENC_SW_PIN) == LOW) encBtnPressed = true;
}

// ================== Task: LED pattern (Core 0) ==================
void taskLEDs(void* pv) {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  uint8_t mode = 0;
  for (;;) {
    // mode LED diganti oleh BTN2 (diset di task tombol lewat notification)
    if (ulTaskNotifyTake(pdTRUE, 0)) {
      mode = (mode + 1) % 3;
    }

    switch (mode) {
      case 0: // chaser
        digitalWrite(LED1_PIN, HIGH); vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED1_PIN, LOW);
        digitalWrite(LED2_PIN, HIGH); vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED2_PIN, LOW);
        digitalWrite(LED3_PIN, HIGH); vTaskDelay(pdMS_TO_TICKS(120));
        digitalWrite(LED3_PIN, LOW);
        break;
      case 1: // blink
        digitalWrite(LED1_PIN, HIGH);
        digitalWrite(LED2_PIN, HIGH);
        digitalWrite(LED3_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(250));
        digitalWrite(LED1_PIN, LOW);
        digitalWrite(LED2_PIN, LOW);
        digitalWrite(LED3_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(250));
        break;
      case 2: // PWM breathing di LED2
        for (int d=0; d<=255; d+=5) {
          analogWrite(LED2_PIN, d);
          vTaskDelay(pdMS_TO_TICKS(8));
        }
        for (int d=255; d>=0; d-=5) {
          analogWrite(LED2_PIN, d);
          vTaskDelay(pdMS_TO_TICKS(8));
        }
        break;
    }
  }
}

// ================== Task: Buttons + Buzzer (Core 0) ==================
void taskButtonsBuzzer(void* pv) {
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  bool buzOn = false;
  for (;;) {
    // BTN1: toggle buzzer
    if (digitalRead(BTN1_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(20)); // debounce
      if (digitalRead(BTN1_PIN) == LOW) {
        buzOn = !buzOn;
        if (buzOn) beepStart(1760); else beepStop();
        while (digitalRead(BTN1_PIN) == LOW) vTaskDelay(pdMS_TO_TICKS(10));
      }
    }

    // BTN2: ganti mode LED (notify task LED)
    if (digitalRead(BTN2_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(20));
      if (digitalRead(BTN2_PIN) == LOW) {
        xTaskNotifyGive(taskLEDsHandle);
        while (digitalRead(BTN2_PIN) == LOW) vTaskDelay(pdMS_TO_TICKS(10));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================== Task: Servo + OLED + Pot (Core 1) ==================
void taskServoOLED(void* pv) {
  // I2C & OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Servo
  servo1.attach(SERVO_PIN);

  for (;;) {
    int raw = analogRead(POT_PIN);
    int angle = map(raw, 0, 4095, 0, 180);
    servo1.write(angle);

    long enc = encCount;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("ESP32-S3 RTOS Demo"));
    display.print(F("Pot: ")); display.print(raw);
    display.print(F("  Ang: ")); display.println(angle);
    display.print(F("Enc: ")); display.println(enc);
    display.print(F("Dir Btn: "));
    display.println(encBtnPressed ? "TOGGLE" : "-");
    display.display();

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================== Task: Stepper (Core 1) ==================
void taskStepper(void* pv) {
  pinMode(ST_A_N, OUTPUT);
  pinMode(ST_A_P, OUTPUT);
  pinMode(ST_B_P, OUTPUT);
  pinMode(ST_B_N, OUTPUT);

  int stepIndex = 0;
  bool dirCW = true;

  for (;;) {
    if (encBtnPressed) {      // tombol encoder untuk ganti arah
      dirCW = !dirCW;
      encBtnPressed = false;
    }

    long speedSteps = abs(encCount);   // semakin diputar semakin cepat
    speedSteps = constrain(speedSteps, 0, 400);
    uint16_t delayMs = 4 + (400 - speedSteps) / 2; // 4..204 ms antar microstep

    // keluarkan urutan step
    const uint8_t* s = STEP_TABLE[stepIndex];
    setStepperCoils(s[0], s[1], s[2], s[3]);

    stepIndex = (dirCW) ? (stepIndex + 1) : (stepIndex + 7);
    stepIndex &= 7;

    vTaskDelay(pdMS_TO_TICKS(delayMs));
  }
}

// ================== Setup & RTOS start ==================
void setup() {
  Serial.begin(115200);
  delay(200);

  // Encoder input + interrupt
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);
  lastA = digitalRead(ENC_A_PIN);
  lastB = digitalRead(ENC_B_PIN);
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), encISR_A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), encISR_B, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_SW_PIN), encBtnISR, FALLING);

  // Buat tasks dan pin ke core yang berbeda
  xTaskCreatePinnedToCore(taskLEDs,        "LEDs",        4096, NULL, 1, &taskLEDsHandle,       0);
  xTaskCreatePinnedToCore(taskButtonsBuzzer,"ButtonsBuzz", 4096, NULL, 2, &taskBtnBuzzHandle,    0);

  xTaskCreatePinnedToCore(taskServoOLED,   "ServoOLED",   6144, NULL, 1, &taskServoOLEDHandle,   1);
  xTaskCreatePinnedToCore(taskStepper,     "Stepper",     4096, NULL, 2, &taskStepperHandle,     1);
}

void loop() {
  // Tidak digunakan karena semua pekerjaan ada di RTOS tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}
