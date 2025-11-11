#include <Arduino.h>
#define SERVO 9
#define CH 2
uint32_t dutyFromUs(uint32_t us){ const uint32_t max=(1<<14)-1; return (uint32_t)((uint64_t)us*max/20000); }
void ServoTask(void *){
  ledcSetup(CH,50,14); ledcAttachPin(SERVO,CH);
  auto writeUs=[&](uint32_t us){ ledcWrite(CH,dutyFromUs(us)); };
  for(;;){
    writeUs(500);  vTaskDelay(pdMS_TO_TICKS(800));
    writeUs(1500); vTaskDelay(pdMS_TO_TICKS(800));
    writeUs(2500); vTaskDelay(pdMS_TO_TICKS(800));
  }
}
void setup(){ xTaskCreatePinnedToCore(ServoTask,"servo",2048,nullptr,2,nullptr,1); }
void loop(){}
