#include <Arduino.h>
#define L1 5
#define L2 6
#define L3 7
void LedTask(void *){
  pinMode(L1,OUTPUT); pinMode(L2,OUTPUT); pinMode(L3,OUTPUT);
  for(;;){
    digitalWrite(L1,!digitalRead(L1)); vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(L2,!digitalRead(L2)); vTaskDelay(pdMS_TO_TICKS(300));
    digitalWrite(L3,!digitalRead(L3)); vTaskDelay(pdMS_TO_TICKS(500));
  }
}
void setup(){ xTaskCreatePinnedToCore(LedTask,"led",2048,nullptr,1,nullptr,0); }
void loop(){}
