#ifndef WIFI_MANAGER_CUSTOM_H
#define WIFI_MANAGER_CUSTOM_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

class WiFiManagerCustom {
public:
    enum class State {
        STARTING,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        PORTAL
    };

    WiFiManagerCustom();

    // Initialize WiFi using stored credentials.
    // If the stored network cannot be reached after several attempts,
    // WiFiManager opens the configuration portal.
    void begin(const char* apName = "Kipas_IoT");

    // Must be called frequently from loop().
    void loop();

    bool connected() const;
    bool portalActive() const;
    bool reconnecting() const;

    State state() const;
    const char* stateName() const;

    String getIP() const;
    String getSSID() const;
    int getRSSI() const;

private:
    WiFiManager wm;
    const char* apName;

    unsigned long lastReconnectAttempt;
    unsigned long reconnectStart;
    unsigned long lastLedBlink;

    static constexpr unsigned long STARTUP_CONNECT_TIMEOUT = 8000UL;
    static constexpr unsigned long RECONNECT_INTERVAL = 10000UL;
    static constexpr unsigned long RECONNECT_TIMEOUT = 8000UL;
    static constexpr unsigned long LED_BLINK_INTERVAL = 500UL;
    static constexpr unsigned long PORTAL_TIMEOUT = 60000UL;

    static constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 5;
    static constexpr uint8_t STARTUP_CONNECT_ATTEMPTS = 5;
    static constexpr uint8_t WIFI_LED_PIN = 2;

    bool ledState;
    bool wasConnected;
    bool reconnectInProgress;
    bool portalIsActive;
    uint8_t reconnectAttempts;

    State currentState;

    bool tryStoredWiFi(unsigned long timeoutMs);
    bool tryConnectionAttempt(unsigned long timeoutMs, uint8_t attempt, const char* phase);

    void startReconnectCycle();
    void reconnect();
    void startConfigPortal();
    void updateLED();
    void setState(State newState);
    void setConnectedState();
};

#endif