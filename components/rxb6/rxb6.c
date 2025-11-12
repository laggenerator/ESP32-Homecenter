#include "rxb6.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#if CONFIG_RXB6_USED

static const char *TAG = "RXB6";

// Kody przycisków z pilota 433MHz
const int przyciski_pilot433mhz[8] = {
    10511624, 10511628, 10511620, 10511625, 
    10511618, 10511621, 10511617, 10511619
};

// Konfiguracja z Kconfig
#define MAX_PULSES 100
#define SYNC_MIN_US 3000
#define SYNC_MAX_US 10000
#define MIN_BITS 24
#define DEBOUNCE_MS CONFIG_RXB6_DEBOUNCE_TIME

// Ustawienie poziomu logowania na podstawie Kconfig
#if CONFIG_RXB6_LOG_LEVEL == 0
    #define RXB6_LOG_LEVEL ESP_LOG_NONE
#elif CONFIG_RXB6_LOG_LEVEL == 1
    #define RXB6_LOG_LEVEL ESP_LOG_ERROR
#elif CONFIG_RXB6_LOG_LEVEL == 2
    #define RXB6_LOG_LEVEL ESP_LOG_WARN
#elif CONFIG_RXB6_LOG_LEVEL == 3
    #define RXB6_LOG_LEVEL ESP_LOG_INFO
#elif CONFIG_RXB6_LOG_LEVEL == 4
    #define RXB6_LOG_LEVEL ESP_LOG_DEBUG
#else
    #define RXB6_LOG_LEVEL ESP_LOG_VERBOSE
#endif

// Zmienne globalne
static volatile uint32_t czasy_impulsow[MAX_PULSES];
static volatile int licznik_impulsow = 0;
static volatile int ostatni_stan = 0;
static volatile uint32_t ostatni_czas = 0;
static volatile uint32_t odebrany_kod = 0;
static volatile bool nowy_kod_available = false;
static volatile int zainicjalizowany_gpio = -1;
static volatile bool component_initialized = false;

// Kolejka do komunikacji między ISR a taskiem
static QueueHandle_t kod_queue = NULL;
static TaskHandle_t receiver_task_handle = NULL;

// ISR handler
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    int stan = gpio_get_level(zainicjalizowany_gpio);
    uint32_t now = esp_timer_get_time();
    
    if (ostatni_czas != 0 && licznik_impulsow < MAX_PULSES) {
        uint32_t czas_trwania = now - ostatni_czas;
        czasy_impulsow[licznik_impulsow++] = czas_trwania;
    }
    
    ostatni_czas = now;
    ostatni_stan = stan;
}

// Funkcja dekodowania impulsów
static void dekodowanie_impulsow(void) {
    if (licznik_impulsow < 10) return;

    int i = 0;
    // Szukanie impulsu synchronizacji
    while (i < licznik_impulsow) {
        if (czasy_impulsow[i] > SYNC_MIN_US && czasy_impulsow[i] < SYNC_MAX_US) {
            break;
        }
        i++;
    }
    
    if (i >= licznik_impulsow) return;

    // Obliczenie opóźnienia na podstawie sync
    int delay = czasy_impulsow[i] / 31;
    if (delay == 0) return;

    uint32_t code = 0;
    int bit_count = 0;

    // Dekodowanie bitów
    for (int j = i + 1; j + 1 < licznik_impulsow; j += 2) {
        uint32_t high = czasy_impulsow[j];
        uint32_t low = czasy_impulsow[j + 1];

        // Logiczne 0: krótki HIGH, długi LOW
        if ((high > delay * 0.8 && high < delay * 1.2) &&
            (low > delay * 2.8 && low < delay * 3.2)) {
            code <<= 1;
            bit_count++;
        }
        // Logiczne 1: długi HIGH, krótki LOW
        else if ((high > delay * 2.8 && high < delay * 3.2) &&
                 (low > delay * 0.8 && low < delay * 1.2)) {
            code = (code << 1) | 1;
            bit_count++;
        } else {
            // Nieprawidłowy impuls - przerwanie dekodowania
            break;
        }
    }

    // Sprawdzenie czy otrzymano pełny kod
    if (bit_count >= MIN_BITS) {
        ESP_LOG_LEVEL(RXB6_LOG_LEVEL, TAG, "Otrzymany kod: %lu", code);
        odebrany_kod = code;
        nowy_kod_available = true;
        
        // Wysłanie kodu przez kolejkę jeśli istnieje
        if (kod_queue != NULL) {
            xQueueSendFromISR(kod_queue, &code, NULL);
        }
    }
}

// Task do przetwarzania danych
static void rxb6_rxTask(void *pvParameter) {
    ESP_LOGI(TAG, "Task odbioru RXB6 uruchomiony");
    
    while (1) {
        if (licznik_impulsow > 10) {
            dekodowanie_impulsow();
            licznik_impulsow = 0;
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t rxb6_init(uint8_t gpio) {
    if (!rxb6_is_configured()) {
        ESP_LOGE(TAG, "RXB6 nie jest włączony w konfiguracji!");
        return ESP_ERR_INVALID_STATE;
    }

    if (component_initialized) {
        ESP_LOGW(TAG, "RXB6 już zainicjalizowany na GPIO %d", zainicjalizowany_gpio);
        return ESP_ERR_INVALID_STATE;
    }

    // Ustawienie poziomu logowania
    esp_log_level_set(TAG, RXB6_LOG_LEVEL);

    zainicjalizowany_gpio = gpio;

    // Konfiguracja GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Błąd konfiguracji GPIO %d: %s", gpio, esp_err_to_name(ret));
        zainicjalizowany_gpio = -1;
        return ret;
    }

    // Instalacja ISR
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Błąd instalacji serwisu ISR: %s", esp_err_to_name(ret));
        zainicjalizowany_gpio = -1;
        return ret;
    }

    // Dodanie handlera ISR
    ret = gpio_isr_handler_add(gpio, gpio_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Błąd dodawania handlera ISR: %s", esp_err_to_name(ret));
        zainicjalizowany_gpio = -1;
        return ret;
    }

    // Utworzenie kolejki
    kod_queue = xQueueCreate(10, sizeof(uint32_t));
    if (kod_queue == NULL) {
        ESP_LOGE(TAG, "Błąd tworzenia kolejki");
        gpio_isr_handler_remove(gpio);
        zainicjalizowany_gpio = -1;
        return ESP_ERR_NO_MEM;
    }

    // Utworzenie taska
    BaseType_t task_ret = xTaskCreate(rxb6_rxTask, "rxb6_receiver", 4096, NULL, 2, &receiver_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Błąd tworzenia taska");
        vQueueDelete(kod_queue);
        kod_queue = NULL;
        gpio_isr_handler_remove(gpio);
        zainicjalizowany_gpio = -1;
        return ESP_FAIL;
    }

    // Reset zmiennych
    licznik_impulsow = 0;
    ostatni_czas = 0;
    odebrany_kod = 0;
    nowy_kod_available = false;
    component_initialized = true;

    ESP_LOGI(TAG, "RXB6 zainicjalizowany na GPIO %d (debounce: %dms)", gpio, DEBOUNCE_MS);
    return ESP_OK;
}

void rxb6_deinit(void) {
    if (!component_initialized) {
        return;
    }

    // Usuwanie ISR
    gpio_isr_handler_remove(zainicjalizowany_gpio);

    // Usuwanie taska
    if (receiver_task_handle != NULL) {
        vTaskDelete(receiver_task_handle);
        receiver_task_handle = NULL;
    }

    // Usuwanie kolejki
    if (kod_queue != NULL) {
        vQueueDelete(kod_queue);
        kod_queue = NULL;
    }

    // Reset zmiennych
    zainicjalizowany_gpio = -1;
    licznik_impulsow = 0;
    ostatni_czas = 0;
    odebrany_kod = 0;
    nowy_kod_available = false;
    component_initialized = false;

    ESP_LOGI(TAG, "RXB6 deinicjalizowany");
}

esp_err_t rxb6_getCode(uint32_t *code) {
    if (code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!component_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!nowy_kod_available) {
        return ESP_ERR_NOT_FOUND;
    }

    *code = odebrany_kod;
    nowy_kod_available = false;

    return ESP_OK;
}

esp_err_t rxb6_whichButton(int *button_num, uint32_t *code) {
    if (button_num == NULL || code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!component_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Sprawdzenie który przycisk został wciśnięty
    for (int i = 0; i < 8; i++) {
        if (*code == przyciski_pilot433mhz[i]) {
            *button_num = i + 1;  // Zwrot numeru przycisku (1-8)
            ESP_LOGI(TAG, "Wykryto przycisk %d", *button_num);
            return ESP_OK;
        }
    }

    // Jeśli kod nie pasuje do żadnego znanego przycisku
    ESP_LOGW(TAG, "Nieznany kod: %lu", *code);
    return ESP_ERR_NOT_FOUND;
}

bool rxb6_is_configured(void) {
    return true; // Zawsze true gdy CONFIG_RXB6_USED jest ustawione
}


void rxb6_przyciskiTask(void *pvParameters){
    uint32_t kod = 0;
    int przycisk_na_pilocie;
    while(1){
        if(rxb6_getCode(&kod) == ESP_OK){
            ESP_LOGE(TAG, "Otrzymano kod: %ul", kod);
            if(rxb6_whichButton(&przycisk_na_pilocie, &kod) == ESP_OK){
                ESP_LOGE(TAG, "Wcisnieto przycisk %d!", przycisk_na_pilocie);
                switch(przycisk_na_pilocie){
                    case 1:
                        ESP_LOGE(TAG, "PRZYCISK1 AKCJA");
                        break;
                    case 8:
                        ESP_LOGE(TAG, "PRZYCISK8 AKCJA");
                        break;
                }
            } else {
                ESP_LOGE(TAG, "Kod nie pasuje do zadnego z przyciskow!");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

#endif // CONFIG_RXB6_USED