#ifndef SINRIC_CTRL_H
#define SINRIC_CTRL_H

#include <Arduino.h>
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "relay_ctrl.h"


/* =====================================================
   SINRIC PRO CONFIGURATION
   ===================================================== */

// WAJIB gunakan credential baru.
// Jangan gunakan credential yang sebelumnya sudah
// terekspos di GitHub.

#define APP_KEY     "d43d4bd8-0241-4997-a46d-e3ffffb0eb5f"
#define APP_SECRET  "5853eab2-8c69-497f-8eaa-fcd9e28d02b8-b97279cc-2f7c-4487-bc10-364eb25f49ac"


/* =====================================================
   SINRIC DEVICE ID
   ===================================================== */

static const char* switchID[3] = {
  "6a8871c829c6be334299415f",
  "6a887202969af7ec248d2b88",
  "6a88721a969af7ec248d2bb7"
};


/* =====================================================
   JUMLAH SWITCH
   ===================================================== */

static const uint8_t SINRIC_SWITCH_COUNT = 3;


/* =====================================================
   STATUS SINRIC
   ===================================================== */

// Callback sudah didaftarkan?
static bool sinricCallbacksRegistered = false;

// SinricPro.begin() sudah pernah dijalankan?
static bool sinricStarted = false;

// Menandai bahwa state relay perlu disinkronkan
// setelah WiFi kembali.
static bool sinricSyncPending = false;


/* =====================================================
   CALLBACK POWER STATE
   ===================================================== */

inline bool onPower(
  const String &deviceId,
  bool &state
) {

  Serial.println();
  Serial.println(
    "[SINRIC] Power command received"
  );

  Serial.print(
    "[SINRIC] Device ID: "
  );

  Serial.println(
    deviceId
  );

  Serial.print(
    "[SINRIC] Requested state: "
  );

  Serial.println(
    state ? "ON" : "OFF"
  );


  /* ===================================================
     CARI DEVICE ID
     =================================================== */

  for (
    uint8_t i = 0;
    i < SINRIC_SWITCH_COUNT;
    i++
  ) {

    if (deviceId == switchID[i]) {

      Serial.print(
        "[SINRIC] Relay target: "
      );

      Serial.println(
        i + 1
      );


      /* ================================================
         ON:
         masuk pending exclusive

         OFF:
         langsung OFF dan membatalkan pending ON
         ================================================ */

      requestExclusive(
        i,
        state,
        switchID
      );

      return true;
    }
  }


  /* ===================================================
     DEVICE TIDAK DIKENAL
     =================================================== */

  Serial.println(
    "[SINRIC] Unknown device ID"
  );

  return false;
}


/* =====================================================
   REGISTER CALLBACK
   HANYA SEKALI
   ===================================================== */

inline void registerSinricCallbacks() {

  if (sinricCallbacksRegistered) {
    return;
  }


  Serial.println(
    "[SINRIC] Registering callbacks..."
  );


  for (
    uint8_t i = 0;
    i < SINRIC_SWITCH_COUNT;
    i++
  ) {

    SinricProSwitch &sw =
      SinricPro[switchID[i]];

    sw.onPowerState(
      onPower
    );
  }


  sinricCallbacksRegistered = true;


  Serial.println(
    "[SINRIC] Callbacks registered"
  );
}


/* =====================================================
   SINKRONISASI STATE RELAY KE SINRIC
   ===================================================== */

inline void syncSinricRelayStates() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }


  Serial.println(
    "[SINRIC] Synchronizing relay states..."
  );


  for (
    uint8_t i = 0;
    i < SINRIC_SWITCH_COUNT;
    i++
  ) {

    bool state =
      getRelayState(i);


    SinricProSwitch &sw =
      SinricPro[switchID[i]];


    sw.sendPowerStateEvent(
      state
    );


    Serial.print(
      "[SINRIC] Relay "
    );

    Serial.print(
      i + 1
    );

    Serial.print(
      " -> "
    );

    Serial.println(
      state ? "ON" : "OFF"
    );
  }


  sinricSyncPending = false;


  Serial.println(
    "[SINRIC] State synchronization requested"
  );
}


/* =====================================================
   INIT SINRIC
   ===================================================== */

inline bool initSinric() {

  /* ===================================================
     WIFI HARUS TERHUBUNG
     =================================================== */

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[SINRIC] WiFi not connected"
    );

    return false;
  }


  Serial.println();
  Serial.println(
    "[SINRIC] Initializing SinricPro..."
  );


  /* ===================================================
     REGISTER CALLBACK
     
     Hanya dilakukan satu kali.
     =================================================== */

  registerSinricCallbacks();


  /* ===================================================
     SinricPro.begin() HANYA SEKALI
     =================================================== */

  if (!sinricStarted) {

    SinricPro.begin(
      APP_KEY,
      APP_SECRET
    );

    sinricStarted = true;


    Serial.println(
      "[SINRIC] SinricPro started"
    );

  } else {

    Serial.println(
      "[SINRIC] SinricPro already started"
    );
  }


  /* ===================================================
     Tandai perlu sinkronisasi
     =================================================== */

  sinricSyncPending = true;


  return true;
}


/* =====================================================
   DIPANGGIL SETELAH WIFI RECONNECT
   ===================================================== */

inline void reconnectSinric() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }


  Serial.println(
    "[SINRIC] WiFi restored"
  );


  /*
     Jangan panggil SinricPro.begin() lagi.

     SinricPro sudah pernah di-start.
     Biarkan handle() menjalankan komunikasi
     dan proses reconnect.
  */

  sinricSyncPending = true;
}


/* =====================================================
   HANDLE SINRIC
   ===================================================== */

inline void handleSinric() {

  /* ===================================================
     WIFI PUTUS
     =================================================== */

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }


  /* ===================================================
     HANDLE SINRIC
     =================================================== */

  if (sinricStarted) {

    SinricPro.handle();
  }


  /* ===================================================
     SINKRONISASI STATE

     Dilakukan setelah SinricPro.handle() diberi
     kesempatan untuk memproses koneksi.
     =================================================== */

  if (sinricSyncPending) {

    static unsigned long lastSyncAttempt = 0;


    /*
       Jangan melakukan request terlalu sering.
    */

    if (
      millis() - lastSyncAttempt >= 2000
    ) {

      lastSyncAttempt = millis();

      syncSinricRelayStates();
    }
  }
}


/* =====================================================
   STATUS SINRIC
   ===================================================== */

inline bool isSinricStarted() {

  return sinricStarted;
}


#endif