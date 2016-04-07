// Basic NTP function for ESP8266 core
// Taken from Arduino examples from ESP8266 package

#ifndef ESP_NTP_H
#define ESP_NTP_H

#include <stdint.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint32_t updateTimeMillis;
} clockTime;

void ntpStart();
unsigned long sendNTPpacket(IPAddress& address);
bool doNTPupdate();
void getCurrTime(clockTime* ct);
unsigned long ct_to_day_seconds(clockTime ct);
#endif
