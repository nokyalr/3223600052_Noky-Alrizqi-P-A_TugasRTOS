#include <Arduino.h>
#define POT 1
void PotTask(void *){
  analogReadResolution(12); analogSetPinAttenuation(POT,ADC_11db);
  for(;;){
    uint16_t raw=analogRead(POT);
    uint8_t pct = map(raw,0,4095,0,100);
    Serial.printf("pot=%u (%u%%)\n",raw,pct);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void setup(){ Serial.begin(115200); xTaskCreatePinnedToCore(PotTask,"pot",2048,nullptr,1,nullptr,1); }
void loop(){}
