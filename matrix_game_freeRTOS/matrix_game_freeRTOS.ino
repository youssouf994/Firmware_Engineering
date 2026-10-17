//#include <Arduino_FreeRTOS.h>
//#include "Arduino_LED_Matrix.h"
#include "matrix_data.h"
#include "matrix_tasks.h"

gestioneMatrice gMatrice;

int bit=0;

void setup() 
{
  
  gMatrice.begin();
  //gestioneMatrice gestione= gestioneMatrice();
  //delay(3000);
}

void loop() 
{
  
}

