#include "WiFiManagerCustom.h"

WiFiManagerCustom::WiFiManagerCustom()
{
    apName = "Kipas_IoT";

    lastReconnectAttempt = 0;
    reconnectStart = 0;
    lastLedBlink = 0;

    ledState = false;
    wasConnected = false;
    reconnectInProgress = false;
    portalIsActive = false;
    reconnectAttempts = 0;

    currentState = State::STARTING;
}

void WiFiManagerCustom::begin(const char* requestedApName)
{
    apName = requestedApName;

    pinMode(WIFI_LED_PIN, OUTPUT);
    ledState = false;
    digitalWrite(WIFI_LED_PIN, LOW);

    lastReconnectAttempt = millis();
    reconnectStart = 0;
    lastLedBlink = millis();

    wasConnected = false;
    reconnectInProgress = false;
    portalIsActive = false;
    reconnectAttempts = 0;

    setState(State::STARTING);

    WiFi.mode(WIFI_STA);

    Serial.println();
    Serial.println("[WIFI] Starting WiFi Manager...");
    Serial.println("[WIFI] Trying stored WiFi...");
    Serial.print("[WIFI] Startup attempts: ");
    Serial.println(STARTUP_CONNECT_ATTEMPTS);

    if (tryStoredWiFi(STARTUP_CONNECT_TIMEOUT))
    {
        setConnectedState();
        return;
    }

    Serial.println("[WIFI] Stored WiFi unavailable after startup retries.");
    startConfigPortal();
}

bool WiFiManagerCustom::tryStoredWiFi(unsigned long timeoutMs)
{
    setState(State::CONNECTING);

    for (uint8_t attempt = 1; attempt <= STARTUP_CONNECT_ATTEMPTS; ++attempt)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }

        if (tryConnectionAttempt(timeoutMs, attempt, "startup"))
        {
            return true;
        }
    }

    return false;
}

bool WiFiManagerCustom::tryConnectionAttempt(
    unsigned long timeoutMs,
    uint8_t attempt,
    const char* phase)
{
    Serial.print("[WIFI] ");
    Serial.print(phase);
    Serial.print(" connection attempt ");
    Serial.print(attempt);

    if (phase[0] == 's')
    {
        Serial.print("/");
        Serial.print(STARTUP_CONNECT_ATTEMPTS);
    }
    else
    {
        Serial.print("/");
        Serial.print(MAX_RECONNECT_ATTEMPTS);
    }

    Serial.println();

    // Do not erase stored credentials. disconnect(false) only disconnects
    // from the current AP and keeps the saved WiFi configuration.
    WiFi.disconnect(false, false);
    delay(100);
    WiFi.begin();

    const unsigned long start = millis();
    lastLedBlink = start;

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < timeoutMs)
    {
        updateLED();
        delay(50);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("[WIFI] ");
        Serial.print(phase);
        Serial.println(" connection successful.");
        return true;
    }

    Serial.print("[WIFI] ");
    Serial.print(phase);
    Serial.print(" connection attempt ");
    Serial.print(attempt);
    Serial.println(" timed out.");

    return false;
}

void WiFiManagerCustom::loop()
{
    const unsigned long now = millis();
    const bool wifiConnected = (WiFi.status() == WL_CONNECTED);

    // The configuration portal is blocking by design. While it is active,
    // do not alter its state from this loop.
    if (portalIsActive)
    {
        return;
    }

    if (wifiConnected)
    {
        if (!wasConnected)
        {
            Serial.println("[WIFI] Connection restored.");
            setConnectedState();
        }
        else
        {
            // Keep the LED solid ON while connected.
            ledState = true;
            digitalWrite(WIFI_LED_PIN, HIGH);
            reconnectInProgress = false;
            reconnectStart = 0;
            reconnectAttempts = 0;
            setState(State::CONNECTED);
        }

        lastReconnectAttempt = now;
        return;
    }

    // Detect a transition from connected -> disconnected.
    if (wasConnected)
    {
        Serial.println("[WIFI] Connection lost.");
        startReconnectCycle();
    }

    updateLED();

    // An active reconnect attempt is allowed to run for RECONNECT_TIMEOUT.
    if (reconnectInProgress)
    {
        if (now - reconnectStart >= RECONNECT_TIMEOUT)
        {
            reconnectInProgress = false;
            reconnectStart = 0;
            lastReconnectAttempt = now;

            if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS)
            {
                Serial.println("[WIFI] Maximum reconnect attempts reached.");
                startConfigPortal();
            }
        }

        return;
    }

    // Keep the reconnect interval explicit so the ESP32 does not repeatedly
    // call WiFi.begin() while the connection is unavailable.
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL)
    {
        reconnect();
    }
}

void WiFiManagerCustom::startReconnectCycle()
{
    wasConnected = false;
    reconnectInProgress = false;
    reconnectStart = 0;
    reconnectAttempts = 0;
    lastReconnectAttempt = millis();
    lastLedBlink = millis();

    setState(State::RECONNECTING);

    ledState = false;
    digitalWrite(WIFI_LED_PIN, LOW);

    Serial.println("[WIFI] Starting reconnect cycle.");
}

void WiFiManagerCustom::reconnect()
{
    if (reconnectInProgress || portalIsActive)
    {
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        setConnectedState();
        return;
    }

    ++reconnectAttempts;
    reconnectInProgress = true;
    reconnectStart = millis();
    lastReconnectAttempt = reconnectStart;

    setState(State::RECONNECTING);

    Serial.print("[WIFI] Starting reconnect attempt ");
    Serial.print(reconnectAttempts);
    Serial.print("/");
    Serial.println(MAX_RECONNECT_ATTEMPTS);

    // Keep stored credentials intact.
    WiFi.disconnect(false, false);
    delay(50);
    WiFi.begin();
}

void WiFiManagerCustom::startConfigPortal()
{
    if (portalIsActive)
    {
        return;
    }

    Serial.println("[WIFI] Opening WiFiManager configuration portal...");
    Serial.print("[WIFI] Portal timeout: ");
    Serial.print(PORTAL_TIMEOUT / 1000UL);
    Serial.println(" seconds");

    reconnectInProgress = false;
    reconnectStart = 0;
    reconnectAttempts = 0;
    portalIsActive = true;
    wasConnected = false;

    setState(State::PORTAL);

    // Portal is intentionally not presented as a connected state.
    ledState = false;
    digitalWrite(WIFI_LED_PIN, LOW);

    wm.setConfigPortalTimeout(PORTAL_TIMEOUT / 1000UL);

    const bool portalResult = wm.startConfigPortal(apName);

    portalIsActive = false;

    if (portalResult && WiFi.status() == WL_CONNECTED)
    {
        Serial.println("[WIFI] WiFi configured successfully from portal.");
        setConnectedState();
        return;
    }

    // Timeout/cancel does not erase stored credentials. Return to the
    // reconnect state instead of opening the portal again immediately.
    wasConnected = false;
    reconnectInProgress = false;
    reconnectStart = 0;
    reconnectAttempts = 0;
    lastReconnectAttempt = millis();
    lastLedBlink = millis();

    setState(State::RECONNECTING);

    Serial.println("[WIFI] Configuration portal closed without connection.");
    Serial.println("[WIFI] Stored credentials were not intentionally cleared.");
    Serial.println("[WIFI] Returning to automatic reconnect cycle.");
}

void WiFiManagerCustom::updateLED()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        ledState = true;
        digitalWrite(WIFI_LED_PIN, HIGH);
        return;
    }

    const unsigned long now = millis();

    if (now - lastLedBlink >= LED_BLINK_INTERVAL)
    {
        lastLedBlink = now;
        ledState = !ledState;
        digitalWrite(WIFI_LED_PIN, ledState ? HIGH : LOW);
    }
}

void WiFiManagerCustom::setState(State newState)
{
    if (currentState == newState)
    {
        return;
    }

    currentState = newState;

    Serial.print("[WIFI] State -> ");
    Serial.println(stateName());
}

void WiFiManagerCustom::setConnectedState()
{
    wasConnected = true;
    reconnectInProgress = false;
    reconnectStart = 0;
    reconnectAttempts = 0;
    lastReconnectAttempt = millis();
    lastLedBlink = millis();

    ledState = true;
    digitalWrite(WIFI_LED_PIN, HIGH);

    setState(State::CONNECTED);

    Serial.print("[WIFI] Connected. IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("[WIFI] RSSI: ");
    Serial.println(WiFi.RSSI());
}

bool WiFiManagerCustom::connected() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManagerCustom::portalActive() const
{
    return portalIsActive;
}

bool WiFiManagerCustom::reconnecting() const
{
    return reconnectInProgress || currentState == State::RECONNECTING;
}

WiFiManagerCustom::State WiFiManagerCustom::state() const
{
    return currentState;
}

const char* WiFiManagerCustom::stateName() const
{
    switch (currentState)
    {
        case State::STARTING:
            return "STARTING";

        case State::CONNECTING:
            return "CONNECTING";

        case State::CONNECTED:
            return "CONNECTED";

        case State::RECONNECTING:
            return "RECONNECTING";

        case State::PORTAL:
            return "PORTAL";

        default:
            return "UNKNOWN";
    }
}

String WiFiManagerCustom::getIP() const
{
    return connected() ? WiFi.localIP().toString() : "0.0.0.0";
}

String WiFiManagerCustom::getSSID() const
{
    return connected() ? WiFi.SSID() : "";
}

int WiFiManagerCustom::getRSSI() const
{
    return connected() ? WiFi.RSSI() : 0;
}