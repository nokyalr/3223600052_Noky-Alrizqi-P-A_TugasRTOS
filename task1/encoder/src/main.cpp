#include <Arduino.h>
#define A_PIN 2
#define B_PIN 3
#define SW_PIN 4
volatile int32_t pos=0; volatile uint8_t lastAB=0;
uint8_t rd(){ return ((uint8_t)digitalRead(B_PIN)<<1)|digitalRead(A_PIN); }
void IRAM_ATTR abIsr(){ uint8_t ab=rd(); uint8_t idx=((lastAB&3)<<2)|(ab&3);
  static const int8_t T[16]={0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
  pos+=T[idx]; lastAB=ab;
}
void IRAM_ATTR swIsr(){ if(digitalRead(SW_PIN)==LOW) Serial.println("SW"); }
void EncTask(void *){
  pinMode(A_PIN,INPUT_PULLUP); pinMode(B_PIN,INPUT_PULLUP); pinMode(SW_PIN,INPUT_PULLUP);
  lastAB=rd(); attachInterrupt(digitalPinToInterrupt(A_PIN),abIsr,CHANGE);
  attachInterrupt(digitalPinToInterrupt(B_PIN),abIsr,CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW_PIN),swIsr,FALLING);
  for(;;){ Serial.printf("pos=%ld\n",(long)pos); vTaskDelay(pdMS_TO_TICKS(200)); }
}
void setup(){ Serial.begin(115200); xTaskCreatePinnedToCore(EncTask,"enc",3072,nullptr,2,nullptr,1); }
void loop(){}
