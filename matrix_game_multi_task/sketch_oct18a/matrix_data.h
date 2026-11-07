#pragma once
#include "Arduino.h"

struct Message //data contract
{
  uint32_t data[3];
};

struct Contract
{
  Message *buffer;
  uint8_t array_position;
};

extern Message valours;
extern Contract contract[3];