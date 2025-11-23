// =====================================================
// ESP32-S3 SIMPLE MUSIC PLAYER (RTOS DEMO)
// FEATURES: Multicore, Queue, Mutex, ISR Button
// COMPONENTS: Buzzer, 2 Buttons, OLED SSD1306
// =====================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -----------------------------------------------------
// PIN MAP
#define BTN_PLAY   4    
#define BTN_NEXT   5   
#define BUZZ_PIN   8    
#define SDA_PIN   10
#define SCL_PIN   11

// -----------------------------------------------------
// GLOBALS
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

QueueHandle_t cmdQ = NULL;        // Queue dari ISR → PlayerTask
SemaphoreHandle_t i2cMtx = NULL;  // Mutex proteksi OLED

enum CmdType { CMD_PLAYPAUSE = 0, CMD_NEXT = 1 };

struct Cmd {
  CmdType type;
};

volatile bool isPlaying = false;
volatile int currentSong = 0;

// -----------------------------------------------------
// ISR BUTTONS (minimal work in ISR)
void IRAM_ATTR isrPlay() {
  if (!cmdQ) return; // safety
  Cmd c{CMD_PLAYPAUSE};
  BaseType_t hp = pdFALSE;
  xQueueSendFromISR(cmdQ, &c, &hp);
  if (hp) portYIELD_FROM_ISR();
}

void IRAM_ATTR isrNext() {
  if (!cmdQ) return; // safety
  Cmd c{CMD_NEXT};
  BaseType_t hp = pdFALSE;
  xQueueSendFromISR(cmdQ, &c, &hp);
  if (hp) portYIELD_FROM_ISR();
}

// -----------------------------------------------------
// MELODY DATA
struct Note { int f; int d; };

Note song0[] = {
  // bar 1: 5 3 1 1 1 - 1 1 7. 1 2
  {392,220}, {330,220}, {262,220}, {262,220}, {262,220},
  {262,220}, {262,220}, {247,300}, {262,220}, {294,380},

  // bar 2: 6 4 2 2 2 - 2 2 1 2 3
  {440,220}, {349,220}, {294,220}, {294,220}, {294,220},
  {294,220}, {294,220}, {262,220}, {294,220}, {330,380},

  // bar 3: 1 3 5 5 5 - 6 5 6 7 1' 1'
  {262,220}, {330,220}, {392,220}, {392,220}, {392,220},
  {440,220}, {392,220}, {440,220}, {494,220}, {523,420}, {523,420},

  // bar 4: 1 1 6 5 5 5 - 4 3 3 2 2 1 1
  {262,220}, {262,220}, {440,220}, {392,220}, {392,220}, {392,220}, 
  {349,220}, {330,220}, {330,220}, {294,220}, {294,220}, {262,420}, {262,420},

  {0,0}
};

Note song1[] = {
  // 3 3 5 1 1
  {330,250}, {330,250}, {392,250}, {262,300}, {262,300},

  // 6 6 1' 4 4
  {440,250}, {440,250}, {523,300}, {349,300}, {349,300},

  // 6 7 5 6 - 7 2' 1' 1'
  {440,250}, {494,250}, {392,250}, {440,250},
  {494,250}, {587,300}, {523,400}, {523,400},

  {0,0}
};

Note song2[] = {
  // bar 1: 5 5 5 6 -  6 6 5 -  6 4 5 5
  {392,250}, {392,250}, {392,250}, {440,250},
  {440,250}, {440,250}, {392,250},
  {440,250}, {349,300}, {392,300}, {392,300},

  // bar 2: 5 1' 1' 1' - 7 6 5 5 - 2' 1' 1'
  {392,250}, {523,300}, {523,300}, {523,300},
  {494,300}, {440,250}, {392,250}, {392,250},
  {587,300}, {523,300}, {523,300},

  // bar 3: 3 4 4 3 - 2 2' 1' 1' 1' - 6 1' 7 2' 1' 1' 1'
  {329,250}, {349,250}, {349,250}, {329,250},
  {293,250}, {587,300}, {523,300}, {523,300}, {523,300},
  {440,250}, {523,300}, {494,300}, {587,300}, {523,400}, {523,400}, {523,400},

  {0,0}
};

Note* songs[] = { song0, song1, song2 };
const int songCount = sizeof(songs) / sizeof(songs[0]);

// -----------------------------------------------------
// SIMPLE TONE FUNCTION FOR ESP32-S3
void playTone(int frequency, int duration) {
  if (frequency <= 0) {
    noTone(BUZZ_PIN);
    delay(duration);
    return;
  }
  
  tone(BUZZ_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZ_PIN);
}

// -----------------------------------------------------
// PLAYER TASK (CORE 1)
void PlayerTask(void *) {
  Cmd cmd;

  for (;;) {
    // Jika tidak playing → tunggu command
    if (!isPlaying) {
      if (xQueueReceive(cmdQ, &cmd, portMAX_DELAY) == pdTRUE) {
        if (cmd.type == CMD_PLAYPAUSE) {
          isPlaying = true;
        } else if (cmd.type == CMD_NEXT) {
          // advance to next song (fixed: increment)
          currentSong = (currentSong + 1) % songCount;
        }
      }
    }

    // Jika playing → jalankan lagu
    if (isPlaying) {
      Note* s = songs[currentSong];
      for (int i = 0; s[i].d != 0; i++) {
        // cek command tanpa blok
        if (xQueueReceive(cmdQ, &cmd, 0) == pdTRUE) {
          if (cmd.type == CMD_PLAYPAUSE) { 
            isPlaying = false; 
            noTone(BUZZ_PIN);
            break; 
          }
          if (cmd.type == CMD_NEXT) { 
            currentSong = (currentSong + 1) % songCount; 
            noTone(BUZZ_PIN);
            break; 
          }
        }

        // Play note using simple tone function
        playTone(s[i].f, s[i].d);
      }
      noTone(BUZZ_PIN);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -----------------------------------------------------
// OLED TASK (CORE 0)
void OledTask(void *) {
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAIL");
    vTaskDelete(NULL);
  }

  for (;;) {
    if (i2cMtx && xSemaphoreTake(i2cMtx, portMAX_DELAY) == pdTRUE) {
      oled.clearDisplay();
      oled.setTextColor(SSD1306_WHITE);
      oled.setTextSize(2);
      oled.setCursor(0,0);
      oled.println("Music RTOS");

      oled.setTextSize(1);
      oled.setCursor(0,28);
      oled.printf("Song: %d\n", currentSong);
      oled.setCursor(0,42);
      oled.printf("State: %s", isPlaying ? "PLAY" : "PAUSE");

      oled.display();
      xSemaphoreGive(i2cMtx);
    }
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

// -----------------------------------------------------
// SETUP
void setup() {
  Serial.begin(115200);

  // create RTOS primitives first (safe to attach ISR afterwards)
  cmdQ = xQueueCreate(10, sizeof(Cmd));
  i2cMtx = xSemaphoreCreateMutex();

  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BUZZ_PIN, OUTPUT);

  // attach ISRs after queue created
  attachInterrupt(digitalPinToInterrupt(BTN_PLAY), isrPlay, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_NEXT), isrNext, FALLING);

  // create tasks
  xTaskCreatePinnedToCore(PlayerTask, "PLAYER", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(OledTask,   "OLED",   4096, NULL, 1, NULL, 0);
}

void loop() {}