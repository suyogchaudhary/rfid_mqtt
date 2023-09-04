#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include "esp_err.h"
#include <stdbool.h>
#define CONNECTED_TO_WIFI 1
#define DISCONNECTED_FROM_WIFI 0
extern volatile bool status;
void wifi_connect_init(void);
esp_err_t wifi_connect_sta(char *ssid, char *pass, int timeout);
void wifi_connect_ap(const char *ssid, const char *pass);
void wifi_disconnect(void);

#endif