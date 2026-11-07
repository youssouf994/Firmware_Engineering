#pragma once
#include "Arduino.h"
#include "matrix_data.h"

class gestore_tasks
{
  private:
    static void carica_frame(void *pvParameters);

  public:
    gestore_tasks();
    void begin();
    static void func_task_frame1(void *pvParameters);
    static void func_task_frame2(void *pvParameters);
    static void func_task_frame3(void *pvParameters);
};


