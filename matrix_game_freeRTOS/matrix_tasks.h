#ifndef MATRIX_TASKS
#define MATRIX_TASKS
#include "matrix_data.h"

class gestioneMatrice
{
  private:
    int LED_BI= 13;

  public:
    gestioneMatrice();
    static void begin();
    static void task_frame(void *pvParameters);
};

void loop_func(void *pvParameters);

#endif