#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SDA_PIN 10
#define SCL_PIN 11
Adafruit_SSD1306 d(128,64,&Wire,-1);
void OledTask(void *){
  Wire.begin(SDA_PIN,SCL_PIN,400000);
  d.begin(SSD1306_SWITCHCAPVCC,0x3C);
  d.clearDisplay(); d.setTextColor(SSD1306_WHITE);
  for(;;){
    d.clearDisplay(); d.setTextSize(2); d.setCursor(0,0); d.print("ESP32-S3");
    d.setTextSize(1); d.setCursor(0,24); d.printf("Uptime %lus",(unsigned long)(millis()/1000));
    d.display(); vTaskDelay(pdMS_TO_TICKS(500));
  }
}
void setup(){ xTaskCreatePinnedToCore(OledTask,"oled",4096,nullptr,2,nullptr,1); }
void loop(){}
