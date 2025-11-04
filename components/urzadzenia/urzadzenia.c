#include "urzadzenia.h"

uint8_t liczba_urzadzen = 0;
Urzadzenie_t urzadzenia[MAX_URZADZEN];

esp_err_t dodajUrzadzenie(const char* nazwa){
  if(liczba_urzadzen >= MAX_URZADZEN){
    ESP_LOGW(TAG_URZADZENIA, "Przekroczono limit urzadzen!");
    return ESP_FAIL;
  }
  strncpy(urzadzenia[liczba_urzadzen].nazwa, nazwa, sizeof(urzadzenia[liczba_urzadzen].nazwa) - 1);
  urzadzenia[liczba_urzadzen].nazwa[sizeof(urzadzenia[liczba_urzadzen].nazwa) - 1] = '\0';
  urzadzenia[liczba_urzadzen].stan = -1;
  liczba_urzadzen++;

  ESP_LOGI(TAG_URZADZENIA, "Dodano urzadzenie %s", nazwa);
  return ESP_OK;
}

void wyswietl_urzadzenia() {
    ESP_LOGI(TAG_URZADZENIA, "\nLista urządzeń (%d):\n", liczba_urzadzen);
    for (int i = 0; i < liczba_urzadzen; i++) {
        ESP_LOGI(TAG_URZADZENIA, "  %d. %s - stan: %d\n", i+1, urzadzenia[i].nazwa, urzadzenia[i].stan);
    }
}