#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#include "mqtt_handler.h"

static const char *TAG = "MQTT_HANDLER";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool connected = false;
static void (*message_callback)(const char*, const char*) = NULL;

#define MAX_TOPICS 10
static char *topics[MAX_TOPICS] = {0};
static int topics_count = 0;

void mqtt_handler_add_topic(bool rodzinny, const char *topic)
{
    if (topics_count >= MAX_TOPICS) {
        ESP_LOGE(TAG, "Max topics reached");
        return;
    }
    
    for (int i = 0; i < topics_count; i++) {
        if (strcmp(topics[i], topic) == 0) return;
    }
    char temat[64];
    if(rodzinny){
        snprintf(temat, sizeof(temat), "%s/%s", CONFIG_MQTT_NAZWA_RODZINY, topic);
    } else {
        snprintf(temat, sizeof(temat), "%s", topic);
    }
    topics[topics_count] = malloc(strlen(temat) + 1);
    if (topics[topics_count] != NULL) {
        strcpy(topics[topics_count], temat);
        topics_count++;
        ESP_LOGI(TAG, "Dodano temat: %s", temat);
    }
}

static void subscribe_all(void)
{
    if (!connected) return;
    
    for (int i = 0; i < topics_count; i++) {
        esp_mqtt_client_subscribe(mqtt_client, topics[i], 1);
        ESP_LOGI(TAG, "Subskrybowano: %s", topics[i]);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                              int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Połączono z brokerem MQTT");
        connected = true;
        subscribe_all();
        mqtt_handler_publish(1, CONFIG_MQTT_KANAL_KEEPALIVE, "ONLINE");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Rozłączono z brokerem MQTT");
        connected = false;
        break;

    case MQTT_EVENT_DATA:
        if (message_callback != NULL) {
            char topic[64] = {0};
            char message[100] = {0};
            
            int topic_len = (event->topic_len < sizeof(topic)-1) ? event->topic_len : sizeof(topic)-1;
            int data_len = (event->data_len < sizeof(message)-1) ? event->data_len : sizeof(message)-1;
            
            memcpy(topic, event->topic, topic_len);
            memcpy(message, event->data, data_len);
            topic[topic_len] = '\0';
            message[data_len] = '\0';
            
            message_callback(topic, message);
        }
        break;

    default:
        break;
    }
}

void mqtt_handler_start(void)
{
    ESP_LOGI(TAG, "Uruchamianie klienta MQTT");
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    
    // Dodaj dane uwierzytelniające jeśli skonfigurowane
    if (strlen(CONFIG_MQTT_USERNAME) > 0) {
        mqtt_cfg.credentials.username = CONFIG_MQTT_USERNAME;
        mqtt_cfg.credentials.authentication.password = CONFIG_MQTT_PASSWORD;
        ESP_LOGI(TAG, "Uwierzytelnianie MQTT: %s", CONFIG_MQTT_USERNAME);
    }

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

bool mqtt_handler_is_connected(void)
{
    return connected;
}

int mqtt_handler_publish(bool rodzinny, const char *topic, const char *message)
{
    if (!connected) return -1;
    int msg_id;
    if(rodzinny){
        char temat[64];
        snprintf(temat, sizeof(temat), "%s/%s", CONFIG_MQTT_NAZWA_RODZINY, topic);
        msg_id = esp_mqtt_client_publish(mqtt_client, temat, message, 0, 1, 0);
        ESP_LOGI(TAG, "Wysłano: %s -> %s", topic, message);
    } else {
        msg_id = esp_mqtt_client_publish(mqtt_client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Wysłano: %s -> %s", topic, message);
    }
    return msg_id;
}

int mqtt_handler_silent_publish(bool rodzinny, const char* topic, const char *message){
    if (!connected) return -1;
    int msg_id;
    if(rodzinny){
        char temat[64];
        snprintf(temat, sizeof(temat), "%s/%s", CONFIG_MQTT_NAZWA_RODZINY, topic);
        msg_id = esp_mqtt_client_publish(mqtt_client, temat, message, 0, 1, 0);
    } else {
        msg_id = esp_mqtt_client_publish(mqtt_client, topic, message, 0, 1, 0);
    }
    return msg_id;
}

void mqtt_handler_set_message_callback(void (*callback)(const char*, const char*))
{
    message_callback = callback;
}