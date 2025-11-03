#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdbool.h>

#define BROADCAST_TOPIC "broadcast"
#define DISCOVERY_TOPIC "discovery"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicjalizacja i uruchomienie klienta MQTT
 */
void mqtt_handler_start(void);

/**
 * @brief Sprawdzenie czy MQTT jest połączony
 * @return true jeśli połączony, false w przeciwnym wypadku
 */
bool mqtt_handler_is_connected(void);

/**
 * @brief Wysłanie wiadomości na temat
 * @param rodzinny Czy wiadomość wysłać tylko w kanale rodzinnym czy wysłać bezpośrednio na topic
 * @param topic Temat na który wysłać wiadomość
 * @param message Wiadomość do wysłania
 * @return ID wiadomości przy sukcesie, -1 przy błędzie
 */
int mqtt_handler_publish(bool rodzinny, const char *topic, const char *message);

/**
 * @brief Wysłanie wiadomości na temat bez logowania wysyłki w ESP
 * @param rodzinny Czy wiadomość wysłać tylko w kanale rodzinnym czy wysłać bezpośrednio na topic
 * @param topic Temat na który wysłać wiadomość
 * @param message Wiadomość do wysłania
 * @return ID wiadomości przy sukcesie, -1 przy błędzie
 */
int mqtt_handler_silent_publish(bool rodzinny, const char *topic, const char *message);

/**
 * @brief Subskrybcja tematu (NIE ZAIMPLEMENTOWANY)
 * @param rodzinny Czy subskrybcja dotyczy kanału rodzinnego czy bezpośrednio topic
 * @param topic Temat do subskrybowania
 * @return ID wiadomości przy sukcesie, -1 przy błędzie
 */
int mqtt_handler_subscribe(bool rodzinny, const char *topic);

/**
 * @brief Subskrybcja wszystkich tematów z tablicy
 * @return liczba subskrybowanych tematów
 */
int mqtt_handler_subscribe_all(void);

/**
 * @brief Ustawienie callbacka dla otrzymanych wiadomości
 * @param callback Wskaźnik funkcji: void callback(const char* topic, const char* message)
 */
void mqtt_handler_set_message_callback(void (*callback)(const char*, const char*));

/**
 * @brief Dodanie tematu do listy subskrybowanych
 * @param rodzinny Czy subskrybcja dotyczy kanału rodzinnego czy bezpośrednio topic
 * @param topic Temat do dodania
 */
void mqtt_handler_add_topic(bool rodzinny, const char *topic);

/**
 * @brief Usunięcie tematu z listy subskrybowanych
 * @param topic Temat do usunięcia
 */
void mqtt_handler_remove_topic(const char *topic);

#ifdef __cplusplus
}
#endif

#endif // MQTT_HANDLER_H