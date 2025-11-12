#ifndef RXB6_H
#define RXB6_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_RXB6_USED

extern const int przyciski_pilot433mhz[8];

/**
 * @brief Inicjalizacja pinu odbierającego dane z RXB6
 * @param gpio numer pina IO
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
*/ 
esp_err_t rxb6_init(uint8_t gpio);

/**
 * @brief Deinicjalizacja pinu odbierającego dane z RXB6
*/ 
void rxb6_deinit(void);

/**
 * @brief Odczyt kodu otrzymanego z RXB6
 * @param code wskaźnik do przechowania surowego kodu z RXB6
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
*/ 
esp_err_t rxb6_getCode(uint32_t *code);

/**
 * @brief Dekodowanie który ze znanych przycisków został wcisnięty
 * @param button_num wskaźnik do przycisku
 * @param code wskaźnik do przechowania surowego kodu z RXB6
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
*/ 
esp_err_t rxb6_whichButton(int *button_num, uint32_t *code);

/**
 * @brief Sprawdza czy komponent RXB6 jest włączony w konfiguracji
 * @return true jeśli włączony, false w przeciwnym razie
 */
bool rxb6_is_configured(void);
/**
 * @brief Task FreeRTOS do obsługi przycisków z pilota 
*/ 
void rxb6_przyciskiTask(void *pvParameters);
#else

// Puste funkcje gdy RXB6 nie jest używany
static inline esp_err_t rxb6_init(uint8_t gpio) { 
    return ESP_ERR_NOT_SUPPORTED; 
}

static inline void rxb6_deinit(void) { }

static inline esp_err_t rxb6_getCode(uint32_t *code) { 
    return ESP_ERR_NOT_SUPPORTED; 
}

static inline esp_err_t rxb6_whichButton(int *button_num, uint32_t *code) { 
    return ESP_ERR_NOT_SUPPORTED; 
}

static inline bool rxb6_is_configured(void) { 
    return false; 
}

#endif // CONFIG_RXB6_USED

#ifdef __cplusplus
}
#endif

#endif // RXB6_H