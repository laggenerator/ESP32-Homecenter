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
#include "guiDraw.h"
#include "gpioManagement.h"
#include "urzadzenia.h"
#if CONFIG_RXB6_USED
#include "rxb6.h"
#endif
#if CONFIG_RYSUNKOWICZ_ENABLE
#include "requesty_http.h"
#endif

#include "setup.h"

static const char *TAG = "MAIN";
static const uint64_t connection_timeout_ms = 5000;
static const uint32_t sleep_time_ms = 5000;
SemaphoreHandle_t xCurrentDeviceMutex, xCurrentDeviceToggleMutex;



typedef struct {
    const char* nazwa;
    uint16_t okres;
} task_config_t;

const przycisk_t przyciski[ILE_PRZYCISKOW] = {
    {Input1, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 1
    {Input2, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 2
    {Input3, GPIO_MODE_INPUT, GPIO_FLOATING},  // Przyisk 3
    {Input4, GPIO_MODE_INPUT, GPIO_FLOATING}   // Przyisk 4
};

void defaultSetupESP32C3(){
    przycisk_t doUstawienia[8] = {
        {GPIO_NUM_2, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_3, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_4, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_5, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_6, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_7, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_8, GPIO_MODE_DISABLE, GPIO_FLOATING},
        {GPIO_NUM_10, GPIO_MODE_DISABLE, GPIO_FLOATING}
    };
    esp_err_t err;
    for(uint8_t i=0;i<8;i++){
        err = setupPrzycisku(&doUstawienia[i]);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Błąd przy wyłączaniu GPIO %d", doUstawienia[i].gpio);
        }
        ESP_LOGI(TAG, "Wyłączanie GPIO %d", doUstawienia[i].gpio);
    }   
    ESP_LOGI(TAG, "Domyślnie wyłączono wszystkie GPIO");
}


QueueHandle_t przyciski_queue = NULL, oled_queue = NULL;
void ustawPrzyciski(przycisk_t *przyciski){
    esp_err_t err;
    for(uint8_t i=0;i < ILE_PRZYCISKOW;i++){
        err = setupPrzycisku(&przyciski[i]);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Błąd przy wyłączaniu GPIO %d", przyciski[i].gpio);
        }
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
    else if (strcmp(temat, discovery_topic) == 0) {
        mqtt_handler_add_topic(1, wiadomosc);
        dodajUrzadzenie(wiadomosc);
        xQueueSend(oled_queue, &(int){0}, portMAX_DELAY);
    }
}

int8_t obecnaPozycjaBtn = 0; // Zmienna od wyboru pozycji przyciskami (góra-dół)

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
        przelaczGPIO(5);
        if(xSemaphoreTake(xCurrentDeviceMutex, portMAX_DELAY) == pdTRUE){
            xSemaphoreGive(xCurrentDeviceMutex);
        }
        ESP_LOGI(TAG, "Przelaczam ;) co %lu ms", dt);
    }
}

void odswiezOLED(void *pvParameters){
    int8_t currIdx = 0, nextIdx = 0, prevIdx = 0;
    uint8_t currVal;
    while(1){
        if(xQueueReceive(oled_queue, &currVal, portMAX_DELAY)){
            switch (currVal){
            case 0:
                oled_clear();
                if(xSemaphoreTake(xCurrentDeviceMutex, portMAX_DELAY) == pdTRUE){
                    currIdx = obecnaPozycjaBtn;
                    xSemaphoreGive(xCurrentDeviceMutex);
                }
                if(liczba_urzadzen != 0){
                    nextIdx = (currIdx+1)%liczba_urzadzen;
                    prevIdx = (currIdx-1)%liczba_urzadzen;
                    if(prevIdx == -1) prevIdx = liczba_urzadzen-1; 
                    // Mutex żeby nie wyświetlić boola "wybranaWartosc" podczas edycji
                    if(xSemaphoreTake(xCurrentDeviceToggleMutex, portMAX_DELAY) == pdTRUE){
                        gui_ekran(&urzadzenia[prevIdx], &urzadzenia[currIdx], &urzadzenia[nextIdx], currIdx, liczba_urzadzen);
                        xSemaphoreGive(xCurrentDeviceToggleMutex);
                    }
                } else {
                    nextIdx = currIdx;
                    prevIdx = currIdx;
                    oled_printf(0, 0, "Nie wykryto");
                    oled_printf(1, 0, "urzadzen");
                    oled_printf(2, 0, "podrzednych!");
                }
                break;
            
            case 1:
                http_getRysunkowicz();
                break;
            case 2:
                oled_clear();
                break;
            }
            // ESP_LOGI(TAG, "Odświeżam ;) (co %lu ms)", dt);
        }
        // vTaskDelayUntil(&xLastWakeTime, xFreq);
        // start_time = esp_timer_get_time() / 1000;
        // dt = start_time - prev_time;
        // prev_time = start_time;
    }
}

void detekcjaPrzyciskow(void *pvParameters){
    bool stany_przyciskow[ILE_PRZYCISKOW] = {DOMYSLNY_STAN};
    bool prev_stany_przyciskow[ILE_PRZYCISKOW] = {DOMYSLNY_STAN};
    esp_err_t err;
    for(uint8_t i=0;i<ILE_PRZYCISKOW;i++){
        err = setupPrzycisku(&przyciski[i]);
        if(err != ESP_OK){
            ESP_LOGE(TAG, "Błąd przy wyłączaniu GPIO %d", przyciski[i].gpio);
        } else {
            ESP_LOGI(TAG, "Ustawiono przycisk %u", i);
        }
    }
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
    int8_t wartosc = 0;
    char wiadomosc[10];
    while(1){
        if(xQueueReceive(przyciski_queue, &currVal, portMAX_DELAY)){
            /* cos w stylu przewijanego menu typu
            gora, dol, (lewo, prawo lub przelacz, potwierdz)
            */
           oled_cmd_t komenda = OLED_DISPLAY;
           switch(currVal){
               case 0: // w góre
                   if(liczba_urzadzen != 0){
                       if(xSemaphoreTake(xCurrentDeviceMutex, portMAX_DELAY) == pdTRUE){
                           obecnaPozycjaBtn = (obecnaPozycjaBtn + 1) % liczba_urzadzen;
                           xSemaphoreGive(xCurrentDeviceMutex);
                       }
                   }
                   break;
               case 1: // w dół
                   if(liczba_urzadzen != 0){
                       if(xSemaphoreTake(xCurrentDeviceMutex, portMAX_DELAY) == pdTRUE){
                           obecnaPozycjaBtn = (obecnaPozycjaBtn - 1) % liczba_urzadzen;
                           if(obecnaPozycjaBtn == -1) obecnaPozycjaBtn = liczba_urzadzen-1;
                           xSemaphoreGive(xCurrentDeviceMutex);
                       }
                   }
                   break;
                case 2: // toggle
                    // Mutex żeby nie zmienić boola/wartosci podczas renderowania
                    if(liczba_urzadzen != 0){
                        if(xSemaphoreTake(xCurrentDeviceToggleMutex, portMAX_DELAY) == pdTRUE){
                            if(urzadzenia[obecnaPozycjaBtn].przelaczany) 
                                urzadzenia[obecnaPozycjaBtn].wybranaWartosc = !urzadzenia[obecnaPozycjaBtn].wybranaWartosc;
                            else
                                urzadzenia[obecnaPozycjaBtn].wybranaWartosc++;
                            xSemaphoreGive(xCurrentDeviceToggleMutex);
                        }
                    }
                    break;
                case 3: // wysyłanie
                    if(liczba_urzadzen != 0){
                        if(xSemaphoreTake(xCurrentDeviceToggleMutex, portMAX_DELAY) == pdTRUE){
                            #if CONFIG_RYSUNKOWICZ_ENABLE
                            if(strcmp(urzadzenia[obecnaPozycjaBtn].nazwa, "Rysunkowicz") == 0){
                                komenda = RYSUNKOWICZ_DISPLAY;
                            } else {
                                wartosc = urzadzenia[obecnaPozycjaBtn].wybranaWartosc;
                                sprintf(wiadomosc, "%d", wartosc);
                                mqtt_handler_publish(1, urzadzenia[obecnaPozycjaBtn].nazwa, wiadomosc);
                            }
                            #else
                            wartosc = urzadzenia[obecnaPozycjaBtn].wybranaWartosc;
                            sprintf(wiadomosc, "%d", wartosc);
                            mqtt_handler_publish(1, urzadzenia[obecnaPozycjaBtn].nazwa, wiadomosc);
                            #endif
                            xSemaphoreGive(xCurrentDeviceToggleMutex);
                        }
                    }
                    break;
            }
            xQueueSend(oled_queue, &komenda, portMAX_DELAY);
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
        }
        
        vTaskDelay(check_interval);
    }
}

// przycisk_t kabel = {16, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY};

void app_main(void)
{
    defaultSetupESP32C3();
    // Inicjalizacja I2C
    i2c_master_bus_handle_t bus_handle;
    i2c_master_init_bus(&bus_handle);
    oled_init(&bus_handle, true);
    oled_clear();
    oled_invert_colors(false);
    if(strcmp(CONFIG_MQTT_NAZWA_URZADZENIA, "zmienmnie") == 0){
        ESP_LOGE(TAG, "Urzadzenie ma domyslna nazwe, zmien ja aby cokolwiek zrobic!");
        oled_printf(0, 0, "Zmien");
        oled_printf(1, 0, "nazwe");
        oled_printf(2, 0, "urzadzenia");
        while(1);
    }
    if(strcmp(CONFIG_MQTT_NAZWA_RODZINY, "zmienmnie") == 0){
        ESP_LOGE(TAG, "Rodzina urzadzen ma domyslna nazwe, zmien ja aby cokolwiek zrobic!");
        oled_printf(0, 0, "Zmien");
        oled_printf(1, 0, "nazwe");
        oled_printf(2, 0, "rodziny");
        oled_printf(3, 0, "urzadzen");
        while(1);
    }
    oled_printf(0, 0, "Inicjalizacja");
    oled_printf(1, 0, "urzadzenia!");
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
    oled_clear();
    oled_printf(0, 0, "Inicjalizacja");
    oled_printf(1, 0, "pamieci NVS!");
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
    oled_clear();
    oled_printf(0, 0, "Inicjalizacja");
    oled_printf(1, 0, "WiFi!");
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
        oled_clear();
        oled_printf(0, 0, "Blad polaczenia");
        oled_printf(1, 0, "z WiFi");
        oled_printf(3, 0, "Sprawdz");
        oled_printf(4, 0, "konfiguracje");
        oled_printf(6, 0, "SSID: %s", CONFIG_WIFI_STA_SSID);
        oled_printf(7, 0, "Haslo: %s", CONFIG_WIFI_STA_PASSWORD);
        vTaskDelay(pdMS_TO_TICKS(1000));
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

    przyciski_queue = xQueueCreate(10, sizeof(uint8_t));
    if(przyciski_queue == NULL){
        ESP_LOGE("System", "Blad przy tworzeniu przyciski_queue!");
        abort();
    }
    oled_queue = xQueueCreate(3, sizeof(uint8_t));
    if(oled_queue == NULL){
        ESP_LOGE("System", "Blad przy tworzeniu oled_queue!");
        abort();
    }
    ESP_LOGI("System", "Inicjalizacja systemu...");
    oled_clear();
    oled_printf(0, 0, "Inicjalizacja");
    oled_printf(1, 0, "systemu!");
    mqtt_init();
    task_config_t przelacznik = {"Przelaczanie SSR", 1000};
    task_config_t ekran = {"Odswiezanie OLED", 1000};
    xCurrentDeviceMutex = xSemaphoreCreateMutex();
    if(xCurrentDeviceMutex == NULL){
        ESP_LOGE("MUTEX", "Błąd przy tworzeniu");
        abort();
    }
    xCurrentDeviceToggleMutex = xSemaphoreCreateMutex();
    if(xCurrentDeviceToggleMutex == NULL){
        ESP_LOGE("MUTEX", "Błąd przy tworzeniu");
        abort();
    }
    #ifdef CONFIG_RXB6_USED
    esp_err_t ret = rxb6_init(4);
    if (ret != ESP_OK) {
        printf("Błąd inicjalizacji RXB6!\n");
        return;
    }
    xTaskCreate(rxb6_przyciskiTask, "RXB6 Przyciski Handler", 2048, NULL, 2, NULL);
    #endif
    xQueueSend(oled_queue, &(int){0}, portMAX_DELAY);
    xTaskCreate(odswiezOLED, "Odswiezanie OLED", 4096, NULL, 2, NULL);
    xTaskCreate(detekcjaPrzyciskow, "Detekcja przyciskow", 2048, NULL, 2, NULL);
    xTaskCreate(przyciskiQueueHandler, "QueueHandler przyciskow", 4096, NULL, 2, NULL);
    #if CONFIG_RYSUNKOWICZ_ENABLE
    dodajUrzadzenie("Rysunkowicz");
    #endif
}
