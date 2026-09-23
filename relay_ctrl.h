#ifndef RELAY_CTRL_H
#define RELAY_CTRL_H

#include <Arduino.h>
#include "eeprom_store.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"

/* =====================================================
   KONFIGURASI RELAY
   ===================================================== */
static const uint8_t RELAY_COUNT = 3;

static const uint8_t relayPin[RELAY_COUNT] = {
  16,
  17,
  18
};

/* =====================================================
   STATE RELAY
   ===================================================== */
static bool relayState[RELAY_COUNT] = {
  false,
  false,
  false
};

/* =====================================================
   EXCLUSIVE STATE
   ===================================================== */
static bool exclusivePending = false;
static int exclusiveTarget = -1;
static uint32_t exclusiveTimer = 0;


/* =====================================================
   INIT RELAY + RESTORE DARI EEPROM
   ===================================================== */
inline void initRelay() {

  eepromStoreBegin();

  // Anti flicker saat boot
  delay(200);

  for (int i = 0; i < RELAY_COUNT; i++) {

    pinMode(relayPin[i], OUTPUT);

    relayState[i] = eepromReadBool(i, false);

    digitalWrite(
      relayPin[i],
      relayState[i] ? HIGH : LOW
    );
  }
}


/* =====================================================
   RELAY CONTROL
   SATU-SATUNYA AKSES UNTUK MENGUBAH RELAY
   ===================================================== */
inline void setRelay(int index, bool on) {

  // Proteksi index
  if (index < 0 || index >= RELAY_COUNT) {
    Serial.print("[RELAY] Invalid index: ");
    Serial.println(index);
    return;
  }

  digitalWrite(
    relayPin[index],
    on ? HIGH : LOW
  );

  eepromWriteBool(
    index,
    on
  );

  relayState[index] = on;

  Serial.print("[RELAY] Relay ");
  Serial.print(index + 1);
  Serial.print(" -> ");
  Serial.println(on ? "ON" : "OFF");
}


/* =====================================================
   GET STATE RELAY
   UNTUK SINKRONISASI SINRIC
   ===================================================== */
inline bool getRelayState(int index) {

  if (index < 0 || index >= RELAY_COUNT) {
    return false;
  }

  return relayState[index];
}


/* =====================================================
   EXCLUSIVE REQUEST
   DIPANGGIL DARI SINRIC
   ===================================================== */
inline void requestExclusive(
  int index,
  bool on,
  const char* switchID[]
) {

  // Proteksi index
  if (index < 0 || index >= RELAY_COUNT) {
    Serial.print("[RELAY] Invalid request index: ");
    Serial.println(index);
    return;
  }


  /* ===================================================
     PERINTAH OFF
     
     PENTING:
     Batalkan pending ON jika masih ada.
     =================================================== */
  if (!on) {

    // Batalkan proses ON yang sedang menunggu
    exclusivePending = false;
    exclusiveTarget = -1;
    exclusiveTimer = 0;

    // Matikan relay yang diminta
    setRelay(index, false);

    // Sinkronisasi ke Sinric
    SinricProSwitch &sw = SinricPro[switchID[index]];
    sw.sendPowerStateEvent(false);

    Serial.print("[RELAY] Relay ");
    Serial.print(index + 1);
    Serial.println(" OFF - pending ON dibatalkan");

    return;
  }


  /* ===================================================
     PERINTAH ON
     =================================================== */

  // Buat pending request baru
  exclusivePending = true;
  exclusiveTarget = index;
  exclusiveTimer = millis();


  // Matikan semua relay lainnya
  for (int i = 0; i < RELAY_COUNT; i++) {

    if (i != index) {
      setRelay(i, false);
    }
  }


  Serial.print("[RELAY] Request ON Relay ");
  Serial.print(index + 1);
  Serial.println(" - menunggu 300 ms");
}


/* =====================================================
   EXCLUSIVE HANDLER
   Menjalankan ON setelah delay 300 ms
   ===================================================== */
inline void handleExclusiveTask(
  const char* switchID[]
) {

  if (!exclusivePending) {
    return;
  }


  /* ===================================================
     CEK TARGET MASIH VALID
     =================================================== */
  if (
    exclusiveTarget < 0 ||
    exclusiveTarget >= RELAY_COUNT
  ) {

    exclusivePending = false;
    exclusiveTarget = -1;
    exclusiveTimer = 0;

    return;
  }


  /* ===================================================
     TUNGGU 300 ms
     =================================================== */
  if (millis() - exclusiveTimer < 300) {
    return;
  }


  /* ===================================================
     NYALAKAN TARGET
     =================================================== */
  int target = exclusiveTarget;

  setRelay(
    target,
    true
  );


  /* ===================================================
     SINKRONISASI SEMUA RELAY KE SINRIC
     =================================================== */
  for (int i = 0; i < RELAY_COUNT; i++) {

    SinricProSwitch &sw =
      SinricPro[switchID[i]];

    sw.sendPowerStateEvent(
      relayState[i]
    );
  }


  /* ===================================================
     SELESAIKAN PENDING
     =================================================== */
  exclusivePending = false;
  exclusiveTarget = -1;
  exclusiveTimer = 0;


  Serial.print("[RELAY] Relay ");
  Serial.print(target + 1);
  Serial.println(" ON - exclusive selesai");
}


#endif