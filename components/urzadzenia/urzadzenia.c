#include "urzadzenia.h"

uint8_t liczba_urzadzen = 0;
Urzadzenie_t urzadzenia[MAX_URZADZEN];

esp_err_t dodajUrzadzenie(const char* nazwa){
  if(liczba_urzadzen >= MAX_URZADZEN){
    ESP_LOGW(TAG_URZADZENIA, "Przekroczono limit urzadzen!");
    return ESP_FAIL;
  }
  char* reszta;
  urzadzenia[liczba_urzadzen].nazwa = strtok_r(nazwa, ";", &reszta);
  urzadzenia[liczba_urzadzen].stan = -1;
  urzadzenia[liczba_urzadzen].wybranaWartosc = 0;
  // trzeba dodac jakies przesyłanie typu nazwa;(toggle/increment)
  if(strcmp(reszta, "toggle") == 0){
    urzadzenia[liczba_urzadzen].przelaczany = 1;
  } else {
    urzadzenia[liczba_urzadzen].przelaczany = 0;
  }
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