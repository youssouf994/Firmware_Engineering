#include <Arduino_FreeRTOS.h>
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrice;
#define LED1 13


TaskHandle_t loop_task, led2_task;

void setup() 
{
  Serial.begin(115200);
  while (!Serial)
  {

  }
  pinMode(LED1, OUTPUT);

  xTaskCreate(loop_thread_func, "loop thread", 512/4, nullptr, 1, &loop_task);
  xTaskCreate(led2_thread_func, "led 2 thread", 512/4, nullptr, 1, &led2_task);

  Serial.println("starting scheduler");
  vTaskStartScheduler();
  for(;;);

  matrice.begin();
}

void loop() 
{
  
}

void loop_thread_func(void *pvParameters)
{
  for (;;)
  {
    loop();
    taskYIELD();
  }
}

void led2_thread_func(void *pvParameters)
{
  pinMode(LED1, OUTPUT);
  for(;;)
  {
    digitalWrite(LED1, !digitalRead(LED1));
    vTaskDelay(pdMS_TO_TICKS(150));
  }
}
