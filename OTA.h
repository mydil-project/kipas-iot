#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

static const char* OTA_HOSTNAME = "Kipas_IoT";
static bool otaCallbacksRegistered = false;
static bool otaStarted = false;
static bool otaReady = false;

inline void registerOTACallbacks() {

  if (otaCallbacksRegistered) {
    return;
  }

  Serial.println(
    "[OTA] Registering callbacks..."
  );

  ArduinoOTA.onStart([]() {

    Serial.println();
    Serial.println(
      "[OTA] Update started"
    );

  });

  ArduinoOTA.onEnd([]() {

    Serial.println();
    Serial.println(
      "[OTA] Update finished"
    );

  });

  ArduinoOTA.onProgress(
    [](unsigned int progress,
       unsigned int total) {

      if (total == 0) {
        return;
      }

      unsigned int percent =
        (progress * 100U) / total;

      Serial.print(
        "\r[OTA] Progress: "
      );

      Serial.print(
        percent
      );

      Serial.print(
        "%"
      );
    }
  );

  ArduinoOTA.onError(
    [](ota_error_t error) {

      Serial.print(
        "\n[OTA] Error["
      );

      Serial.print(
        error
      );

      Serial.print(
        "]: "
      );

      if (error == OTA_AUTH_ERROR) {

        Serial.println(
          "Authentication Failed"
        );

      }

      else if (error == OTA_BEGIN_ERROR) {

        Serial.println(
          "Begin Failed"
        );

      }

      else if (error == OTA_CONNECT_ERROR) {

        Serial.println(
          "Connect Failed"
        );

      }

      else if (error == OTA_RECEIVE_ERROR) {

        Serial.println(
          "Receive Failed"
        );

      }

      else if (error == OTA_END_ERROR) {

        Serial.println(
          "End Failed"
        );

      }

      else {

        Serial.println(
          "Unknown Error"
        );
      }
    }
  );

  otaCallbacksRegistered = true;

  Serial.println(
    "[OTA] Callbacks registered"
  );
}

inline bool initOTA() {

  if (WiFi.status() != WL_CONNECTED) {

    otaReady = false;

    return false;
  }

  Serial.println();
  Serial.println(
    "[OTA] Initializing OTA..."
  );

  ArduinoOTA.setHostname(
    OTA_HOSTNAME
  );

  registerOTACallbacks();

  if (!otaStarted) {

    ArduinoOTA.begin();

    otaStarted = true;

    Serial.println(
      "[OTA] ArduinoOTA started"
    );

  }
  else {

    Serial.println(
      "[OTA] ArduinoOTA already started"
    );
  }

  otaReady = true;

  Serial.print(
    "[OTA] Ready. Hostname: "
  );

  Serial.println(
    OTA_HOSTNAME
  );

  Serial.print(
    "[OTA] IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );

  return true;
}

inline void handleOTA() {

  if (WiFi.status() != WL_CONNECTED) {

    otaReady = false;

    return;
  }

  if (!otaReady) {

    initOTA();

    return;
  }

  if (!otaStarted) {

    initOTA();

    return;
  }

  ArduinoOTA.handle();
}

inline void reconnectOTA() {

  if (WiFi.status() != WL_CONNECTED) {

    otaReady = false;

    return;
  }

  Serial.println(
    "[OTA] WiFi restored"
  );

  otaReady = true;

  Serial.print(
    "[OTA] OTA available at "
  );

  Serial.println(
    WiFi.localIP()
  );
}

inline bool isOTAReady() {

  return otaReady;
}

inline bool areOTACallbacksRegistered() {

  return otaCallbacksRegistered;
}

inline bool isOTAStarted() {

  return otaStarted;
}

#endif