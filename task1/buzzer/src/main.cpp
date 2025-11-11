#include <Arduino.h>
#define BZ 1
#define CH 0
void BuzzTask(void *){
  ledcSetup(CH,1000,10); ledcAttachPin(BZ,CH);
  for(;;){
    ledcWriteTone(CH,1000); ledcWrite(CH,512); vTaskDelay(pdMS_TO_TICKS(150));
    ledcWrite(CH,0);        vTaskDelay(pdMS_TO_TICKS(80));
    ledcWriteTone(CH,1500); ledcWrite(CH,512); vTaskDelay(pdMS_TO_TICKS(180));
    ledcWrite(CH,0);        vTaskDelay(pdMS_TO_TICKS(800));
  }
}
void setup(){ xTaskCreatePinnedToCore(BuzzTask,"buzz",2048,nullptr,2,nullptr,1); }
void loop(){}
