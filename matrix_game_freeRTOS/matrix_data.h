#ifndef MATRIX_DATA
#define MATRIX_DATA
#include "Arduino.h"

struct Frame
{
  uint32_t dati[3];
};

extern Frame frame;

#endif