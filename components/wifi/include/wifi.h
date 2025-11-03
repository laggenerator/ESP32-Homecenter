#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

/**
 * @brief Event group bits for WiFi events
 */
#define WIFI_STA_CONNECTED_BIT BIT0
#define WIFI_STA_IPV4_OBTAINED_BIT BIT1
#define WIFI_STA_IPV6_OBTAINED_BIT BIT2

/**
 * @brief Inicjalizacja WiFi w trybie STA
 * 
 * Trzeba odpalić esp_netif_init() i esp_event_loop_create_default() zanim odpala się to
 * 
 * @param[in] event_group Event group handle for WiFi and IP events. Pass NULL to use the existing event group.
 * 
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
 */ 
esp_err_t wifi_sta_init(EventGroupHandle_t event_group);

/**
 * @brief Wyłącz WiFi
 * 
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
 */
esp_err_t wifi_sta_stop(void);

/**
 * @brief Restart WiFi
 * 
 * @return 
 *  - ESP_OK przy sukcesie
 *  - inne przy błędzie
 */
esp_err_t wifi_sta_reconnect(void);



#endif