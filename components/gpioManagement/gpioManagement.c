#include "gpioManagement.h"

/**
 * @brief Ustawia przycisk na wybrany tryb, domyślnie ustawia pulldown output
 * @param gpio - numer pina IO
 * @param kierunek - GPIO_MODE_OUTPUT lub GPIO_MODE_INPUT
 * @param tryb - GPIO_PULLUP_ONLY, GPIO_PULLDOWN_ONLY, GPIO_FLOATING, GPIO_PULLUP_PULLDOWN
 */ 
esp_err_t setupPrzycisku(przycisk_t *przycisk){
  esp_err_t err;
  esp_rom_gpio_pad_select_gpio(przycisk->gpio);
  err = gpio_set_direction(przycisk->gpio, przycisk->kierunek);
  if(err != ESP_OK) return err;
  switch (przycisk->tryb) {
    case GPIO_PULLUP_PULLDOWN:
      err = gpio_pullup_en(przycisk->gpio);
      if (err == ESP_OK) err = gpio_pulldown_en(przycisk->gpio);
      break;
    case GPIO_PULLUP_ONLY:
      err = gpio_pullup_en(przycisk->gpio);
      if (err == ESP_OK) err = gpio_pulldown_dis(przycisk->gpio);
      break;
    case GPIO_PULLDOWN_ONLY:
        err = gpio_pulldown_en(przycisk->gpio);
      if (err == ESP_OK) err = gpio_pullup_dis(przycisk->gpio);
      break;
    default: // Floating
      err = gpio_pullup_dis(przycisk->gpio);
      if (err == ESP_OK) err = gpio_pulldown_dis(przycisk->gpio);
      break;
  }
  
  return err;
}

/**
 * @brief Przełącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t przelaczGPIO(gpio_num_t gpio){
  uint8_t lvl = gpio_get_level(gpio);
  esp_err_t err;
  if(lvl == 0) err = gpio_set_level(gpio, 1); 
  else err = gpio_set_level(gpio, 0);
  return err;
};

/**
 * @brief Włącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t wlaczGPIO(gpio_num_t gpio){
  esp_err_t err = gpio_set_level(gpio, 1);
  return err;
};

/**
 * @brief Wyłącza output pina
 * @param gpio - numer pina IO
 */
esp_err_t wylaczGPIO(gpio_num_t gpio){
  esp_err_t err = gpio_set_level(gpio, 0);
  return err;
};