
#ifndef WIFOSCAN_H
#define WIFOSCAN_H

// Define a structure to hold details for acceptable wifi networks
typedef struct {
    const char* ssid;
    const char* password;
    const char* broker;
} wifi_info_t;

#endif // WIFOSCAN_H


const wifi_info_t *get_wifi_info();
