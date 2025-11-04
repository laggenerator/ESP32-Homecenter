#ifndef URZADZENIA_H
#define URZADZENIA_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define MAX_URZADZEN 20
#define TAG_URZADZENIA "Urzadzenia"

typedef struct {
  char nazwa[50];
  int8_t stan;
} Urzadzenie_t;

extern uint8_t liczba_urzadzen;
extern Urzadzenie_t urzadzenia[MAX_URZADZEN];

esp_err_t dodajUrzadzenie(const char* nazwa);
void wyswietl_urzadzenia();

#endif