#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "wifi.h"
#include "mqtt_handler.h"
#include "i2ctest.h"
#include "ssd1306.h"
#include "gpioManagement.h"

#include "setup.h"

// ZŁE BO INNE ESP pozdro
#define GPIO_
#define GPIO_ENABLE_REG  (*(volatile uint32_t *)0x3FF44020)
#define GPIO_OUT_REG     (*(volatile uint32_t *)0x3FF44004)

static const char *TAG = "XYZ";
static const uint64_t connection_timeout_ms = 10000;
static const uint32_t sleep_time_ms = 5000;
SemaphoreHandle_t xZmiennaMutex;

void wlaczSwiatlo(uint8_t gpio){
  GPIO_OUT_REG |= (1<<gpio);
}

void wylaczSwiatlo(uint8_t gpio){
  GPIO_OUT_REG &= ~(1<<gpio);
}

void przelaczSwiatlo(uint8_t gpio){
  GPIO_OUT_REG ^= (1<<gpio);
}


typedef struct {
    const char* nazwa;
    uint16_t okres;
} task_config_t;

przycisk_t przyciski[ILE_PRZYCISKOW] = {
    {Input1, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 1
    {Input2, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 2
    {Input3, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 3
    {Input4, GPIO_MODE_INPUT, GPIO_FLOATING}   // Przyisk 4
};

QueueHandle_t przyciski_queue = NULL;

void ustawPrzyciski(przycisk_t *przyciski){
    for(uint8_t i=0;i < ILE_PRZYCISKOW;i++){
        setupPrzycisku(&przyciski[i]);
    }
};

void obsluzWiadomosciMQTT(const char* temat, const char* wiadomosc){
    printf("Otrzymano MQTT - Temat: %s, Wiadomość: %s\n", temat, wiadomosc);
    char status_topic[64];
    char broadcast_topic[64];
    char discovery_topic[64];
    snprintf(broadcast_topic, sizeof(broadcast_topic), "%s/broadcast", CONFIG_MQTT_NAZWA_RODZINY);
    snprintf(discovery_topic, sizeof(discovery_topic), "%s/discovery", CONFIG_MQTT_NAZWA_RODZINY);
    snprintf(status_topic, sizeof(status_topic), "%s/%s", CONFIG_MQTT_NAZWA_RODZINY, CONFIG_MQTT_KANAL_KEEPALIVE);
    
    if (strcmp(temat, broadcast_topic) == 0) {
        if (strcmp(wiadomosc, "start") == 0) {
            printf("Otrzymano wiadomosc broadcast!\n");
            mqtt_handler_publish(1, "discovery", CONFIG_MQTT_NAZWA_URZADZENIA);
        }
    }
}

uint16_t zmienna = 0;
void PrzelaczSSR(void *pvParameters){
    task_config_t *config = (task_config_t *)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFreq = pdMS_TO_TICKS(config->okres);
    int64_t start_time = 0, prev_time = 0;
    uint16_t dt = 0;
    while(1){
        vTaskDelayUntil(&xLastWakeTime, xFreq);
        start_time = esp_timer_get_time() / 1000;
        dt = start_time - prev_time;
        prev_time = start_time;
        // przelaczSwiatlo(5);
        przelaczGPIO(5);
        oled_clear();
        if(xSemaphoreTake(xZmiennaMutex, portMAX_DELAY) == pdTRUE){
            oled_printf(0, 0, "Zmienna: %u", zmienna);
            xSemaphoreGive(xZmiennaMutex);
        }
        oled_printf(1, 0, "dt: %lu ms", dt);
        ESP_LOGI(TAG, "Przelaczam ;) co %lu ms", dt);
    }
}

void przyciskTask(void *pvParameters){
    przycisk_t *przycisk = (przycisk_t *)pvParameters;
    setupPrzycisku(przycisk);
    bool obecnyStan = 0, poprzedniStan = 1;
    while(1){
        obecnyStan = gpio_get_level(przycisk->gpio);
        if(obecnyStan != poprzedniStan){
            if(xSemaphoreTake(xZmiennaMutex, portMAX_DELAY) == pdTRUE){
                zmienna++;
                xSemaphoreGive(xZmiennaMutex);
            }
        }
        poprzedniStan = obecnyStan;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void detekcjaPrzyciskow(void *pvParameters){
    bool stany_przyciskow[ILE_PRZYCISKOW] = {DOMYSLNY_STAN};
    bool prev_stany_przyciskow[ILE_PRZYCISKOW] = {DOMYSLNY_STAN};
    while(1){
        for(uint8_t i=0;i<ILE_PRZYCISKOW;i++){
            stany_przyciskow[i] = gpio_get_level(przyciski[i].gpio);
            if(stany_przyciskow[i] != DOMYSLNY_STAN && stany_przyciskow[i] != prev_stany_przyciskow[i]){
                xQueueSend(przyciski_queue, &i, portMAX_DELAY);
            }
            prev_stany_przyciskow[i] = stany_przyciskow[i];
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void przyciskiQueueHandler(void *pvParameters){
    uint8_t currVal;
    while(1){
        if(xQueueReceive(przyciski_queue, &currVal, portMAX_DELAY)){
            /* cos w stylu przewijanego menu typu 
            switch(currVal){
                case 0: w dol
                current_item = (current_item -1)%ile_itemow
                case 1: w gore
                current_item = (current_item +1)%ile_itemow
                case 2: toggle prawo (lub toggle)
                case 3: toggle lewo (lub send do toggle wyzszego)
            }
            */
        }
    }
}

void MQTTKeepAliveTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFreq = pdMS_TO_TICKS(60*1000);
    while(1) {
        if(mqtt_handler_is_connected()){
            mqtt_handler_silent_publish(1, "status", CONFIG_MQTT_NAZWA_URZADZENIA);
        }
        vTaskDelayUntil(&xLastWakeTime, xFreq); // Co 60 sekund
    }
}

void mqtt_init(){
#if CONFIG_MQTT_MASTER
    mqtt_handler_add_topic(1, "discovery");
#else
    mqtt_handler_add_topic(1, "broadcast");
#endif
    mqtt_handler_set_message_callback(obsluzWiadomosciMQTT);
    mqtt_handler_start();
    xTaskCreate(MQTTKeepAliveTask, "MQTT Keep Alive", 2048, NULL, 1, NULL);
}

void wifi_monitor_task(void *pvParameters){
    EventGroupHandle_t network_event_group = (EventGroupHandle_t)pvParameters;
    const TickType_t check_interval = pdMS_TO_TICKS(sleep_time_ms);

    while(1){
        EventBits_t bits = xEventGroupGetBits(network_event_group);

        if(!(bits & WIFI_STA_CONNECTED_BIT)) {
            ESP_LOGW(TAG, "WiFi rozlaczone - probuje sie znowu podlaczyc");
            
            esp_err_t ret = wifi_sta_reconnect();
            if(ret != ESP_OK) {
                ESP_LOGE(TAG, "Ponowne laczenie nie wyszlo, sprobuje za %d", 
                         check_interval / portTICK_PERIOD_MS);
            }
        } else {
            ESP_LOGI(TAG, "Jestem tutaj :)");
        }
        
        vTaskDelay(check_interval);
    }
}

przycisk_t kabel = {16, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY};

void app_main(void)
{
    if(strcmp(CONFIG_MQTT_NAZWA_URZADZENIA, "zmienmnie") == 0){
        ESP_LOGE(TAG, "Urzadzenie ma domyslna nazwe, zmien ja aby cokolwiek zrobic!");
        while(1);
    }
    if(strcmp(CONFIG_MQTT_NAZWA_RODZINY, "zmienmnie") == 0){
        ESP_LOGE(TAG, "Rodzina urzadzen ma domyslna nazwe, zmien ja aby cokolwiek zrobic!");
        while(1);
    }
    esp_err_t esp_ret;
    EventGroupHandle_t network_event_group;
    EventBits_t network_event_bits;
    // Inicjalizacja event group
    network_event_group = xEventGroupCreate();
    if(network_event_group == NULL){
        ESP_LOGE(TAG, "Blad przy inicjalizacji grupy eventow");
        abort();
    }
    // Inicjalizacja NVS
    esp_ret = nvs_flash_init(); 
    if((esp_ret == ESP_ERR_NVS_NO_FREE_PAGES) ||
        (esp_ret == ESP_ERR_NVS_NEW_VERSION_FOUND)){
            ESP_ERROR_CHECK(nvs_flash_erase());
            esp_ret = nvs_flash_init();
    }
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad (%d): Blad inicjalizacji NVS", esp_ret);
        abort();
    }
    // Inicjalizacja interfejsu TCP/IP
    esp_ret = esp_netif_init();
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad (%d): Blad przy inicjalizacji interfejsu sieciowego", esp_ret);
        abort();
    }

    esp_ret = esp_event_loop_create_default();
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad (%d): Blad przy tworzeniu domyslnej petli wydarzen", esp_ret);
        abort();
    }

    // Inicjalizacja polaczenia WiFi
    esp_ret = wifi_sta_init(network_event_group);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad (%d): Blad przy inicjalizacji polaczenia sieciowego WiFi", esp_ret);
        abort();
    }

    ESP_LOGI(TAG, "Oczekiwanie na polaczenie sieciowe...");
    network_event_bits = xEventGroupWaitBits(network_event_group, 
                                            WIFI_STA_CONNECTED_BIT, 
                                            pdFALSE, pdTRUE, 
                                            pdMS_TO_TICKS(connection_timeout_ms));
    if(network_event_bits & WIFI_STA_CONNECTED_BIT){
        ESP_LOGI(TAG, "Polaczono z siecia WiFI");
    } else {
        ESP_LOGE(TAG, "Blad przy laczeniu z siecia WiFi");
        abort();
    }

    ESP_LOGI(TAG, "Oczekiwanie na adres sieciowy...");
    network_event_bits = xEventGroupWaitBits(network_event_group, 
                                            WIFI_STA_IPV4_OBTAINED_BIT | WIFI_STA_IPV6_OBTAINED_BIT, 
                                            pdFALSE, pdFALSE, 
                                            pdMS_TO_TICKS(connection_timeout_ms));
    if(network_event_bits & WIFI_STA_IPV4_OBTAINED_BIT){
        ESP_LOGI(TAG, "Zdobyto adres IPv4");
    } else if(network_event_bits & WIFI_STA_IPV6_OBTAINED_BIT){
        ESP_LOGI(TAG, "Zdobyto adres IPv6"); 
    } else {
        ESP_LOGE(TAG, "Blad przy zdobywaniu adresu IP");
        abort();
    }
    xTaskCreate(wifi_monitor_task, "Monitoring WiFi", 4096, network_event_group, 1, NULL);

    // Inicjalizacja I2C
    i2c_master_bus_handle_t bus_handle;
    i2c_master_init_bus(&bus_handle);
    przyciski_queue = xQueueCreate(10, sizeof(uint8_t));
    if(przyciski_queue == NULL){
        ESP_LOGE("System", "Blad przy tworzeniu przyciski_queue!");
        abort();
    }
    ESP_LOGI("System", "Inicjalizacja systemu...");
    mqtt_init();
    oled_init(&bus_handle, true);
    setupPrzycisku(&kabel);
    printf("\n\nhalo\n\n");
    GPIO_ENABLE_REG |= (1<<5);
    task_config_t przelacznik = {"Przelaczanie SSR", 1000};
    xZmiennaMutex = xSemaphoreCreateMutex();
    if(xZmiennaMutex == NULL){
        ESP_LOGE("MUTEX", "Błąd przy tworzeniu");
        abort();
    }
    xTaskCreate(PrzelaczSSR, "Przelaczanie SSR", 4096, &przelacznik, 2, NULL);
    xTaskCreate(przyciskTask, "Przycisk1", 2048, &kabel, 2, NULL);
}
