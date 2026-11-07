#include "Arduino.h"
#include <Arduino_FreeRTOS.h>
#include "Arduino_LED_Matrix.h"
#include "matrix_data.h"
#include "matrix_tasks.h"

TaskHandle_t task_loader, task_frame1, task_frame2, task_frame3;
ArduinoLEDMatrix matrix;


gestore_tasks::gestore_tasks()
{
  
}

void gestore_tasks::begin()
{
  matrix.begin();
  
  contract[0].array_position=0;
  contract[1].array_position=1;
  contract[2].array_position=2;

  contract[0].buffer->data[0]=0x000000FF;
  contract[1].buffer->data[1]=0x0000FF00;
  contract[2].buffer->data[2]=0x00FF0000;

  matrix.loadFrame(contract[0].buffer->data);
  delay(3000);

  xTaskCreate(func_task_frame1, "frame 1", 512/4, &contract[0], 2, &task_frame1);
  xTaskCreate(func_task_frame2, "task 2", 512/4, &contract[1], 2, &task_frame2);
  xTaskCreate(func_task_frame3, "task 3", 512/4, &contract[2], 2, &task_frame3);
  xTaskCreate(carica_frame, "loader", 512/4, &contract[0], 4, &task_loader);
  vTaskStartScheduler();
}


void gestore_tasks::func_task_frame1(void *pvParameters)
{
  Contract *m = static_cast<Contract*>(pvParameters);

  for(;;)
  {
    m->buffer->data[m->array_position]= 0xFFFFFFFF;
    vTaskDelay(pdMS_TO_TICKS(750));

    m->buffer->data[m->array_position]= 0x00000000;
    vTaskDelay(pdMS_TO_TICKS(750));
  }
}

void gestore_tasks::func_task_frame2(void *pvParameters)
{
  Contract *m = static_cast<Contract*>(pvParameters);
  uint32_t *w = &m->buffer->data[m->array_position];

  for(;;)
  { 
    *w=0;
    for(int bit=0;bit <32;bit++)
    {
      *w |= (1UL << bit);
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
}

void gestore_tasks::func_task_frame3(void *pvParameters)
{
  Contract *m = static_cast<Contract*>(pvParameters);
  for(;;)
  {
    m->buffer->data[m->array_position]= 0x00000000;
    vTaskDelay(pdMS_TO_TICKS(750));

    m->buffer->data[m->array_position]= 0xFFFFFFFF;
    vTaskDelay(pdMS_TO_TICKS(750));
  }
}

void gestore_tasks::carica_frame(void *pvParameters)
{
  Contract *m = static_cast<Contract*>(pvParameters);
  for(;;)
  {
    matrix.loadFrame(m->buffer->data);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}



