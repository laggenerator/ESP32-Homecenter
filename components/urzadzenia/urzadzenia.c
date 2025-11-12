#include "urzadzenia.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

uint8_t liczba_urzadzen = 0;
Urzadzenie_t urzadzenia[MAX_URZADZEN];

// esp_err_t dodajUrzadzenie(const char* nazwa){
//   if(liczba_urzadzen >= MAX_URZADZEN){
//     ESP_LOGW(TAG_URZADZENIA, "Przekroczono limit urzadzen!");
//     return ESP_FAIL;
//   }
//   char* reszta;
//   urzadzenia[liczba_urzadzen].nazwa = strtok_r(nazwa, ";", &reszta);
//   urzadzenia[liczba_urzadzen].stan = -1;
//   urzadzenia[liczba_urzadzen].wybranaWartosc = 0;
//   // trzeba dodac jakies przesyłanie typu nazwa;(toggle/increment)
//   if(strcmp(reszta, "toggle") == 0){
//     urzadzenia[liczba_urzadzen].przelaczany = 1;
//   } else {
//     urzadzenia[liczba_urzadzen].przelaczany = 0;
//   }
//   liczba_urzadzen++;

//   ESP_LOGI(TAG_URZADZENIA, "Dodano urzadzenie %s", nazwa);
//   return ESP_OK;
// }

esp_err_t dodajUrzadzenie(const char* nazwa){
    if(liczba_urzadzen >= MAX_URZADZEN){
        ESP_LOGW(TAG_URZADZENIA, "Przekroczono limit urzadzen!");
        return ESP_FAIL;
    }
    
    char kopia_nazwy[64];
    strncpy(kopia_nazwy, nazwa, sizeof(kopia_nazwy)-1);
    kopia_nazwy[sizeof(kopia_nazwy)-1] = '\0';
    
    char* reszta;
    char* token = strtok_r(kopia_nazwy, ";", &reszta);
    
    if(token == NULL) {
        ESP_LOGE(TAG_URZADZENIA, "Brak nazwy w: %s", nazwa);
        return ESP_FAIL;
    }
    
    urzadzenia[liczba_urzadzen].nazwa = malloc(strlen(token) + 1);
    if(urzadzenia[liczba_urzadzen].nazwa == NULL) {
        ESP_LOGE(TAG_URZADZENIA, "Brak pamieci dla nazwy");
        return ESP_FAIL;
    }
    strcpy(urzadzenia[liczba_urzadzen].nazwa, token);
    
    urzadzenia[liczba_urzadzen].stan = -1;
    urzadzenia[liczba_urzadzen].wybranaWartosc = 0;
    
    if(reszta != NULL && strcmp(reszta, "toggle") == 0){
        urzadzenia[liczba_urzadzen].przelaczany = 1;
        ESP_LOGI(TAG_URZADZENIA, "Urzadzenie %s - typ: toggle", token);
    } else {
        urzadzenia[liczba_urzadzen].przelaczany = 0;
        if(reszta != NULL) 
          ESP_LOGI(TAG_URZADZENIA, "Urzadzenie %s - typ: increment", token);
    }
    
    liczba_urzadzen++;
    ESP_LOGI(TAG_URZADZENIA, "Dodano urzadzenie: %s (liczba: %d)", urzadzenia[liczba_urzadzen-1].nazwa, liczba_urzadzen);
    return ESP_OK;
}

void wyswietl_urzadzenia() {
    ESP_LOGI(TAG_URZADZENIA, "\nLista urządzeń (%d):\n", liczba_urzadzen);
    for (int i = 0; i < liczba_urzadzen; i++) {
        ESP_LOGI(TAG_URZADZENIA, "  %d. %s - stan: %d\n", i+1, urzadzenia[i].nazwa, urzadzenia[i].stan);
    }
}