#ifndef REQUESTY_HTTP_H
#define REQUESTY_HTTP_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"

static esp_err_t _http_event_handler(esp_http_client_event_t *event);
void http_getRysunkowicz(void);

#endif