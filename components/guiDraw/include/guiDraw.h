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
#include "urzadzenia.h"
void gui_ekran(Urzadzenie_t* gorne_pole, Urzadzenie_t* srodkowe_pole, Urzadzenie_t* dolne_pole, uint8_t currentItemNum, uint8_t allItemsCount);
void gui_scroll(uint8_t currentItemNum, uint8_t allItemsCount);

#endif