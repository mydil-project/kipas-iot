#ifndef EEPROM_STORE_H
#define EEPROM_STORE_H

#include <Arduino.h>
#include <EEPROM.h>

/* =====================================================
   KONFIGURASI EEPROM
   ===================================================== */

#define EEPROM_SIZE 64

// Alamat penyimpanan state relay
#define EEPROM_RELAY_1 0
#define EEPROM_RELAY_2 1
#define EEPROM_RELAY_3 2


/* =====================================================
   STATUS EEPROM
   ===================================================== */

static bool eepromInitialized = false;


/* =====================================================
   INIT EEPROM
   ===================================================== */

inline void eepromStoreBegin() {

  if (eepromInitialized) {
    return;
  }

  if (!EEPROM.begin(EEPROM_SIZE)) {

    Serial.println(
      "[EEPROM] Initialization failed"
    );

    return;
  }

  eepromInitialized = true;

  Serial.println(
    "[EEPROM] Initialized"
  );
}


/* =====================================================
   MENDAPATKAN ALAMAT RELAY
   ===================================================== */

inline int getRelayEEPROMAddress(int index) {

  switch (index) {

    case 0:
      return EEPROM_RELAY_1;

    case 1:
      return EEPROM_RELAY_2;

    case 2:
      return EEPROM_RELAY_3;

    default:
      return -1;
  }
}


/* =====================================================
   BACA BOOLEAN DARI EEPROM
   ===================================================== */

inline bool eepromReadBool(
  int index,
  bool defaultValue = false
) {

  /* ===================================================
     Pastikan EEPROM sudah aktif
     =================================================== */

  if (!eepromInitialized) {
    eepromStoreBegin();
  }


  /* ===================================================
     VALIDASI INDEX
     =================================================== */

  int address =
    getRelayEEPROMAddress(index);

  if (address < 0) {

    Serial.print(
      "[EEPROM] Invalid read index: "
    );

    Serial.println(
      index
    );

    return defaultValue;
  }


  /* ===================================================
     BACA DATA
     =================================================== */

  uint8_t value =
    EEPROM.read(address);


  /* ===================================================
     VALIDASI DATA

     Hanya 0 dan 1 yang dianggap valid.
     =================================================== */

  if (value == 0) {
    return false;
  }

  if (value == 1) {
    return true;
  }


  /* ===================================================
     DATA EEPROM BELUM VALID / KORUP
     =================================================== */

  Serial.print(
    "[EEPROM] Invalid data at address "
  );

  Serial.print(
    address
  );

  Serial.print(
    ", value = "
  );

  Serial.println(
    value
  );

  return defaultValue;
}


/* =====================================================
   TULIS BOOLEAN KE EEPROM
   ===================================================== */

inline bool eepromWriteBool(
  int index,
  bool value
) {

  /* ===================================================
     Pastikan EEPROM aktif
     =================================================== */

  if (!eepromInitialized) {
    eepromStoreBegin();
  }


  /* ===================================================
     VALIDASI INDEX
     =================================================== */

  int address =
    getRelayEEPROMAddress(index);

  if (address < 0) {

    Serial.print(
      "[EEPROM] Invalid write index: "
    );

    Serial.println(
      index
    );

    return false;
  }


  /* ===================================================
     CEK DATA LAMA
     
     Jika nilainya sama, tidak perlu menulis EEPROM.
     =================================================== */

  uint8_t newValue =
    value ? 1 : 0;

  uint8_t oldValue =
    EEPROM.read(address);


  if (oldValue == newValue) {
    return true;
  }


  /* ===================================================
     TULIS DATA
     =================================================== */

  EEPROM.write(
    address,
    newValue
  );


  /* ===================================================
     COMMIT
     =================================================== */

  if (!EEPROM.commit()) {

    Serial.print(
      "[EEPROM] Commit failed at address "
    );

    Serial.println(
      address
    );

    return false;
  }


  Serial.print(
    "[EEPROM] Relay "
  );

  Serial.print(
    index + 1
  );

  Serial.print(
    " saved = "
  );

  Serial.println(
    value ? "ON" : "OFF"
  );


  return true;
}


#endif