#include <Arduino.h>
#define STEP_PIN 12
#define DIR_PIN 13
const int stepsPerRev = 200;
volatile int stepDelayUS = 1000; // 1 kHz/2 edge = ~500 step/s

void StepperTask(void *)
{
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  for (;;)
  {
    digitalWrite(DIR_PIN, HIGH);
    for (int i = 0; i < stepsPerRev; i++)
    {
      digitalWrite(STEP_PIN, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(STEP_PIN, LOW);
      ets_delay_us(stepDelayUS);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(DIR_PIN, LOW);
    for (int i = 0; i < stepsPerRev; i++)
    {
      digitalWrite(STEP_PIN, HIGH);
      ets_delay_us(stepDelayUS);
      digitalWrite(STEP_PIN, LOW);
      ets_delay_us(stepDelayUS);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void setup() { xTaskCreatePinnedToCore(StepperTask, "step", 2048, nullptr, 2, nullptr, 0); }
void loop() {}
