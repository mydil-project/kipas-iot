#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <Arduino.h>
#include <WiFi.h>
#include "WiFiManagerCustom.h"

#define LED_WIFI 2
#define WIFI_AP_NAME "Kipas_IoT"

static WiFiManagerCustom wifiManagerCustom;
static bool wifiInitialized = false;

inline void initWiFi()
{
    Serial.println();
    Serial.println("[WIFI] Initializing custom WiFi Manager...");

    wifiManagerCustom.begin(WIFI_AP_NAME);
    wifiInitialized = wifiManagerCustom.connected();

    if (wifiInitialized)
    {
        Serial.println("[WIFI] Connected");
        Serial.print("[WIFI] IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("[WIFI] Not connected after WiFi Manager startup.");
    }
}

inline void handleWiFiReconnect()
{
    wifiManagerCustom.loop();
    wifiInitialized = wifiManagerCustom.connected();
}

inline void wifiStatusLED()
{
    // LED is handled by WiFiManagerCustom::loop().
    // Keep this function for compatibility with main.ino.
}

inline bool isWiFiConnected()
{
    return wifiManagerCustom.connected();
}

#endif