#include "matrix_tasks.h"
#include <Arduino_FreeRTOS.h>
#include "Arduino_LED_Matrix.h"
#include "matrix_data.h"

ArduinoLEDMatrix matrix;
TaskHandle_t loop_task, frame_task;


gestioneMatrice::gestioneMatrice()
{

  pinMode(LED_BI, OUTPUT);
   
}

void gestioneMatrice::begin()
{
  matrix.begin();
  xTaskCreate(loop_func, "loop func", 512/4, nullptr, 1, &loop_task);
  xTaskCreate(task_frame, "thread 0 func", 512/4, &frame, 1, &frame_task);
  vTaskStartScheduler();
}
void loop_func(void *pvParameters)
{
  for(;;)
  {
    loop();
    taskYIELD();
    //vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void gestioneMatrice::task_frame(void *pvParameters)
{
  Frame *f = static_cast<Frame*>(pvParameters);
  for(;;)
  {
    int bit;
    for(bit=0;bit<32;bit++)
    {
      f->dati[0] |= (1UL << bit);
      f->dati[1] |= (1UL << bit);
      f->dati[2] |= (1UL << bit);

      vTaskDelay(pdMS_TO_TICKS(250));
      matrix.loadFrame(f->dati);
    }

    uint8_t conta =0;
    
    for(conta=0;conta<3;++conta)
    {
      f->dati[0]= 0x00000000;
      f->dati[1]= 0x00000000;
      f->dati[2]= 0x00000000; 
      matrix.loadFrame(f->dati);  
      vTaskDelay(pdMS_TO_TICKS(250));  
      f->dati[0]= 0xFFFFFFFF;
      f->dati[1]= 0xFFFFFFFF;
      f->dati[2]= 0xFFFFFFFF;

      matrix.loadFrame(f->dati);
      vTaskDelay(pdMS_TO_TICKS(250));
    }

    for(bit=32;bit>=0;bit--)
    {
      f->dati[0]&= ~(1UL << bit);
      f->dati[1]&= ~(1UL << bit);
      f->dati[2]&= ~(1UL << bit);

      matrix.loadFrame(f->dati);
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
}

