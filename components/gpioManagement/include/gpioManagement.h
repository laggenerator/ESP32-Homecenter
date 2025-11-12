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

/**
 * @brief Ustawia przycisk na wybrany tryb, domyślnie ustawia pulldown output
 * @param gpio - numer pina IO
 * @param kierunek - GPIO_MODE_OUTPUT lub GPIO_MODE_INPUT
 * @param tryb - GPIO_PULLUP_ONLY, GPIO_PULLDOWN_ONLY, GPIO_FLOATING, GPIO_PULLUP_PULLDOWN
 */ 
esp_err_t setupPrzycisku(przycisk_t *przycisk);

/**
 * @brief Przełącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t przelaczGPIO(gpio_num_t gpio);

/**
 * @brief Włącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t wlaczGPIO(gpio_num_t gpio);

/**
 * @brief Wyłącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t wylaczGPIO(gpio_num_t gpio);


#endif