#ifndef GPIOMANAGEMENT_H
#define GPIOMANAGEMENT_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <driver/gpio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

typedef struct {
  gpio_num_t gpio;
  gpio_mode_t kierunek;
  gpio_pull_mode_t tryb;
} przycisk_t;

esp_err_t setupPrzycisku(przycisk_t *przycisk);
esp_err_t przelaczGPIO(gpio_num_t gpio);
esp_err_t wlaczGPIO(gpio_num_t gpio);
esp_err_t wylaczGPIO(gpio_num_t gpio);


#endif