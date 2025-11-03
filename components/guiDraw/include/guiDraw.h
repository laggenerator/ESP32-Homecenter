#ifndef GUIDRAW_H
#define GUIDRAW_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "ssd1306.h"

typedef struct {
  const char* nazwa;
  uint8_t stan;
} Pole_t;

void gui_ekran(Pole_t* gorne_pole, Pole_t* srodkowe_pole, Pole_t* dolne_pole, uint8_t currentItemNum, uint8_t allItemsCount);
void gui_scroll(uint8_t currentItemNum, uint8_t allItemsCount);

#endif