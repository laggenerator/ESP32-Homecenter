#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_netif.h"
#include "esp_log.h"
#include "esp_private/wifi.h"

#include "wifi.h"
// TAG
static const char *TAG = "WIFI";

// Statyczne zmienne globalne
static esp_netif_t *s_wifi_netif = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static wifi_netif_driver_t s_wifi_driver = NULL;

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_start(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *data);


static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    switch(event_id){
        case WIFI_EVENT_STA_START:
            if(s_wifi_netif != NULL){
                wifi_start(s_wifi_netif, event_base, event_id, event_data);
            }
            break;
        case WIFI_EVENT_STA_STOP:
            if(s_wifi_netif != NULL){
                esp_netif_action_stop(s_wifi_netif, event_base, event_id, event_data);
            }
            break;
        case WIFI_EVENT_STA_CONNECTED:
            if(s_wifi_netif == NULL){
                ESP_LOGE(TAG, "WiFi nie wystartowalo, interface handle jest NULL");
                return;
            }

            wifi_event_sta_connected_t *event_sta_connected = (wifi_event_sta_connected_t *)event_data;
            ESP_LOGI(TAG, "Połączono z AP");
            ESP_LOGI(TAG, " SSID: %s", (char*)event_sta_connected->ssid);
            ESP_LOGI(TAG, " Kanal: %d", event_sta_connected->channel);
            ESP_LOGI(TAG, " Tryb auth: %d", event_sta_connected->authmode);
            ESP_LOGI(TAG, " AID: %d", event_sta_connected->aid);

            wifi_netif_driver_t driver = esp_netif_get_io_driver(s_wifi_netif);
            if(!esp_wifi_is_if_ready_when_started(driver)){
                esp_err_t esp_ret = esp_wifi_register_if_rxcb(driver, esp_netif_receive, s_wifi_netif);
                if(esp_ret != ESP_OK){
                    ESP_LOGE(TAG, "Blad w rejestracji WiFi RX callback");
                    return;
                }
            }

            esp_netif_action_connected(s_wifi_netif, event_base, event_id, event_data);

            xEventGroupSetBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);

#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
            esp_ret = esp_netif_create_ip6_linklocal(s_wifi_netif);
            if(esp_ret != ESP_OK){
                ESP_LOGE(TAG, "Blad przy tworzeniu adresu IPv6 link-local");
            }
#endif
            break;

            case WIFI_EVENT_STA_DISCONNECTED:
            if(s_wifi_netif != NULL){
                esp_netif_action_disconnected(s_wifi_netif, event_base, event_id, event_data);
            }
            xEventGroupClearBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
#if CONFIG_WIFI_STA_AUTO_RECONNECT
                ESP_LOGI(TAG, "Proba ponownego polaczenia z siecia");
                wifi_sta_reconnect();
#endif
                break;
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    esp_err_t esp_ret;
    switch (event_id){
#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    case IP_EVENT_STA_GOT_IP:
        if(s_wifi_netif == NULL){
            ESP_LOGE(TAG, "Blad przy nabyciu adresu IPv4, interface handle jest NULL");
            return;
        }
        esp_ret = esp_wifi_internal_set_sta_ip();
        if(esp_ret != ESP_OK){
            ESP_LOGE(TAG, "Blad przy przeslaniu adresu IP do drivera WiFi");
        }

        xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
        ip_event_got_ip_t *event_ip = (ip_event_got_ip_t *)event_data;
        esp_netif_ip_info_t *ip_info = &event_ip->ip_info;
        ESP_LOGI(TAG, "Adres IPv4 zdobyty");
        ESP_LOGI(TAG, " IP: ", IPSTR, IP2STR(&ip_info->ip));
        ESP_LOGI(TAG, " Maska: ", IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, " Gateway: ", IPSTR, IP2STR(&ip_info->gw));
        break;
#endif
        
#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
        case IP_EVENT_GOT_IP6:
            if(s_wifi_netif == NULL){
                ESP_LOGE(TAG, "Blad przy nabyciu adresu IPv6, interface handle jest NULL");
                return;
            }

            esp_ret = esp_wifi_internal_set_sta_ip();
            if(esp_ret != ESP_OK){
                ESP_LOGE(TAG, "Blad przy przeslaniu adresu IP do drivera");
            }

            xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
            ip_event_got_ip6_t *event_ipv6 = (ip_event_got_ip6_t *)event_data;
            esp_netif_ip6_info_t *ip6_info = &event_ipv6->ip6_info;
            ESP_LOGI(TAG, "Adres IPv6 zdobyty");
            ESP_LOGI(TAG, " IP: ", IPV62STR, IPV6STR(ip6info->ip));
            ESP_LOGI(TAG, " Maska: ", IPV62STR, IPV6STR(ip6info->netmask));
            ESP_LOGI(TAG, " Gateway: ", IPV62STR, IPV6STR(ip6_info->gw));
            break;

#endif

            case IP_EVENT_STA_LOST_IP:
                xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
                xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
                ESP_LOGI(TAG, "Adres IP stracony");
                break;

            default:
                ESP_LOGI(TAG, "Niezhandlowany event IP: %li", event_id);
                break;
    }
}

static void wifi_start(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *data){
    uint8_t mac_addr[6] = {0};
    esp_err_t esp_ret;

    // Zdobycie esp-netif driver handle
    wifi_netif_driver_t driver = esp_netif_get_io_driver(esp_netif);
    if(driver == NULL){
        ESP_LOGE(TAG, "Blad przy zdobywaniu drivera wifi");
        return;
    }
    // Zdobywanie adresu MAC interfejsu
    esp_ret = esp_wifi_get_if_mac(driver, mac_addr);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy zdobywaniu adresu MAC");
        return;
    }
    // Printowanie adresu MAC
    ESP_LOGI(TAG, "Adres MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    // Rejestrowanie referencji netstack buffera i free callback
    esp_ret = esp_wifi_internal_reg_netstack_buf_cb(esp_netif_netstack_buf_ref, esp_netif_netstack_buf_free);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad (%d): Netstack callback registration fail", esp_ret);
        return;
    }
    // Przypisanie adresu MAC do interfejsu WiFi
    esp_ret = esp_netif_set_mac(esp_netif, mac_addr);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy ustawianiu adresu MAC");
        return;
    }
    // Start interfejsu WiFi
    esp_netif_action_start(esp_netif, event_base, event_id, data);

    // Łączenie z WiFi
    ESP_LOGI(TAG, "Laczenie z %s: ", CONFIG_WIFI_STA_SSID);
    esp_ret = esp_wifi_connect();
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad podczas laczenia z siecia");
    }
}

/*******************************************/
// Funkcje publiczne

// Inicjalizacja WiFI w trybie STA
esp_err_t wifi_sta_init(EventGroupHandle_t event_group){
    esp_err_t esp_ret;
    ESP_LOGI(TAG, "Startowanie WiFi w trybie STA");
    if(event_group != NULL){
        s_wifi_event_group = event_group;
    }

    if(s_wifi_event_group == NULL){
        ESP_LOGE(TAG, "Event group handle jest NULL");
        return ESP_FAIL;
    }

    // Tworzenie domyślnego interfejsu WiFi
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_WIFI_STA();
    s_wifi_netif = esp_netif_new(&netif_cfg);
    if(s_wifi_netif == NULL){
        ESP_LOGE(TAG, "Blad przy tworzeniu interfejsu WiFi");
        return ESP_FAIL;
    }

    // Tworzenie drivera interfejsu
    s_wifi_driver = esp_wifi_create_if_driver(WIFI_IF_STA);
    if(s_wifi_driver == NULL){
        ESP_LOGE(TAG, "Blad przy tworzeniu drivera WiFi");
        return ESP_FAIL;
    }
    // Tworzenie powiązania między interfejsem sieciowym i driverem
    esp_ret = esp_netif_attach(s_wifi_netif, s_wifi_driver);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy powiazywaniu drivera to interfejsu");
        return ESP_FAIL;
    }

    // Rejestracja eventów

    // IP event: STA zostało wystartowane
    esp_ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &on_wifi_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu WiFi STA START event handler");
        return ESP_FAIL;
    }

    // IP event: STA zostało zatrzymane
    esp_ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &on_wifi_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu WiFi STA STOP event handler");
        return ESP_FAIL;
    }

    // IP event: STA zostało połączone
    esp_ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu WiFi STA CONNECTED event handler");
        return ESP_FAIL;
    }

    // IP event: STA zostało rozłączone
    esp_ret = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu WiFi STA DISCONNECTED event handler");
        return ESP_FAIL;
    }

#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED

    // IP event: STA dostało IPv4
    esp_ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu STA GOT IP event handler");
        return ESP_FAIL;
    }
#endif

#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED

    // IP event: STA dostało IPV6
    esp_ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP6, &on_ip_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu STA GOT IP6 event handler");
        return ESP_FAIL;
    }
#endif
    // IP event: STA straciło adres
    esp_ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, &on_wifi_event, NULL);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu STA LOST IP event handler");
        return ESP_FAIL;
    }
    
    // Shutdown handler
    esp_ret = esp_register_shutdown_handler((shutdown_handler_t)esp_wifi_stop);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy rejestrowaniu SHUTDOWN HANDLER");
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_ret = esp_wifi_init(&cfg);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy inicjalizacji WiFi");
        return ESP_FAIL;
    }

    esp_ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if(esp_ret != ESP_OK){
        ESP_LOGE(TAG, "Blad przy ustawianiu trybu STA");
        return ESP_FAIL;
    }

#if CONFIG_WIFI_STA_AUTH_OPEN
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
#elif CONFIG_WIFI_STA_AUTH_WEP
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WEP;
#elif CONFIG_WIFI_STA_AUTH_WPA_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA2_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA_WPA2_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA_WPA2_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA3_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA3_PSK;
#elif CONFIG_WIFI_STA_AUTH_WPA2_WPA3_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_WPA3_PSK;
#elif CONFIG_WIFI_STA_AUTH_WAPI_PSK
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_WAPI_PSK;
#else
    const wifi_auth_mode_t auth_mode = WIFI_AUTH_OPEN;
#endif

    // Konfiguracja WPA3 SAE
#if CONFIG_WIFI_STA_WPA3_SAE_PWE_HUNT_AND_PECK
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_HUNT_AND_PECK;
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_H2E
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_HASH_TO_ELEMENT;
#elif CONFIG_WIFI_STA_WPA3_SAE_PWE_BOTH
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_BOTH;
#else
    const wifi_sae_pwe_method_t sae_pwe_method = WPA3_SAE_PWE_UNSPECIFIED;
#endif

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_STA_SSID,
            .password = CONFIG_WIFI_STA_PASSWORD,
            .threshold.authmode = auth_mode,
            .sae_pwe_h2e = sae_pwe_method,
            .sae_h2e_identifier = CONFIG_WIFI_STA_WPA3_PASSWORD_ID,
        },
    };
    esp_ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy konfiguracji WiFi");
        return ESP_FAIL;
    }

    // Start WiFi
    esp_ret = esp_wifi_start();
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy starcie drivera WiFi");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Zatrzymywanie WiFi
esp_err_t wifi_sta_stop(void)
{
    esp_err_t esp_ret;

    ESP_LOGI(TAG, "Zatrzymywanie WiFi...");

    // Odrejestrowywanie eventów
    // station start
    esp_ret = esp_event_handler_unregister(WIFI_EVENT, 
                                          WIFI_EVENT_STA_START, 
                                          &on_wifi_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu WiFi STA start event handler");
        return ESP_FAIL;
    }

    // station stop
    esp_ret = esp_event_handler_unregister(WIFI_EVENT, 
                                          WIFI_EVENT_STA_STOP, 
                                          &on_wifi_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu WiFi STA stop event handler");
        return ESP_FAIL;
    }

    // station connected
    esp_ret = esp_event_handler_unregister(WIFI_EVENT, 
                                          WIFI_EVENT_STA_CONNECTED, 
                                          &on_wifi_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu WiFi STA connected event handler");
        return ESP_FAIL;
    }

    // station disconnected
    esp_ret = esp_event_handler_unregister(WIFI_EVENT, 
                                          WIFI_EVENT_STA_DISCONNECTED, 
                                          &on_wifi_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu WiFi STA disconnected event " \
                      "handler");
        return ESP_FAIL;
    }

#if CONFIG_WIFI_STA_CONNECT_IPV4 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    // STA dostało adres IPv4
    esp_ret = esp_event_handler_unregister(IP_EVENT, 
                                          IP_EVENT_STA_GOT_IP, 
                                          &on_ip_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu IPv4 event handler");
        return ESP_FAIL;
    }
#endif

#if CONFIG_WIFI_STA_CONNECT_IPV6 || CONFIG_WIFI_STA_CONNECT_UNSPECIFIED
    // STA dostało adres IPv6
    esp_ret = esp_event_handler_unregister(IP_EVENT, 
                                          IP_EVENT_GOT_IP6, 
                                          &on_ip_event);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu IPv6 event handler");
        return ESP_FAIL;
    }
#endif

    // shutdown handler
    esp_ret = esp_unregister_shutdown_handler(
        (shutdown_handler_t)esp_wifi_stop);
    if (esp_ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Shutdown handler juz byl usuniety");
    } else if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy usuwaniu shutdown handler");
        return ESP_FAIL;
    }

    // Rozłączanie z WiFi
    esp_ret = esp_wifi_disconnect();
    if (esp_ret == ESP_ERR_WIFI_NOT_INIT || ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGI(TAG, "WiFi juz jest rozlaczone");
    } else if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad (%d): Blad przy rozlaczaniu od sieci WiFi", esp_ret);
        return ESP_FAIL;
    }

    // Zatrzymywanie drivera WiFi
    esp_ret = esp_wifi_stop();
    if (esp_ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "WiFi driver juz jest zatrzmany");
    } else if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad (%d): Blad przy zatrzymywaniu WiFi driver", esp_ret);
        return ESP_FAIL;
    }

    // Unloadowanie drivera i zwalnianie zasobów
    esp_ret = esp_wifi_deinit();
    if (esp_ret == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGI(TAG, "WiFi driver juz zostal zdeinicjalizowany");
    } else if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad (%d): Blad przy deinicjalizowaniu WiFi", esp_ret);
        return ESP_FAIL;
    }

    // Odczepianie i zwalnianie drivera WiFi
    if (s_wifi_driver != NULL) {
        esp_wifi_destroy_if_driver(s_wifi_driver);
        s_wifi_driver = NULL;
    }

    // Usuwanie interfejsu
    if (s_wifi_netif != NULL) {
        esp_netif_destroy(s_wifi_netif);
    }

    // Czyszczenie grupy eventów
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_CONNECTED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
    }

    // Ustawianie handle na NULL
    s_wifi_netif = NULL;

    ESP_LOGI(TAG, "WiFi zatrzmane");

    return ESP_OK;
}

// Reconnectowanie
esp_err_t wifi_sta_reconnect(void)
{
    esp_err_t esp_ret;

    // Zatrzymanie WiFi
    esp_ret = wifi_sta_stop();
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy zatrzymywaniu WiFi podczas ponownego laczenia");
        return esp_ret;
    }

    // Start WiFi
    esp_ret = wifi_sta_init(NULL);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Blad przy starcie WiFi podczas ponownego laczenia");
        return esp_ret;
    }

    return ESP_OK;
}