#ifndef SETUP_H
#define SETUP_H

#include "esp_err.h"
#include "gpioManagement.h"

#define Input1 4
#define Input2 5
#define Input3 6
#define Input4 7
#define ILE_PRZYCISKOW 4
#define DOMYSLNY_STAN 0

typedef enum {
  OLED_DISPLAY,
  RYSUNKOWICZ_DISPLAY,
  OLED_CLEAR
} oled_cmd_t;

#endif