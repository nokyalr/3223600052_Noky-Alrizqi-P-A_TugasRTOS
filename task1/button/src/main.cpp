#include <Arduino.h>
#define BTN 4
QueueHandle_t q; struct Ev{bool pressed; uint32_t ms;};
void IRAM_ATTR isr(){ bool p=(digitalRead(BTN)==LOW); uint32_t t=millis(); Ev e{p,t}; BaseType_t hp=pdFALSE; xQueueSendFromISR(q,&e,&hp); if(hp) portYIELD_FROM_ISR(); }
void BtnTask(void *){
  pinMode(BTN,INPUT_PULLUP);
  for(Ev e;;){ if(xQueueReceive(q,&e,portMAX_DELAY)==pdTRUE){ if(e.pressed) Serial.println("PRESSED"); else Serial.println("RELEASED"); } }
}
void setup(){ Serial.begin(115200); q=xQueueCreate(8,sizeof(Ev)); attachInterrupt(digitalPinToInterrupt(BTN),isr,CHANGE); xTaskCreatePinnedToCore(BtnTask,"btn",2048,nullptr,2,nullptr,1); }
void loop(){}
