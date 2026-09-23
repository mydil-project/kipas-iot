#ifndef TIME_NTP_H
#define TIME_NTP_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

/* =====================================================
   KONFIGURASI WAKTU
   ===================================================== */

// WIB = UTC+7
#define GMT_OFFSET_SEC  (7 * 3600)
#define DAYLIGHT_OFFSET_SEC 0


/* =====================================================
   NTP SERVER
   ===================================================== */

static const char* NTP_SERVER_1 = "pool.ntp.org";
static const char* NTP_SERVER_2 = "time.nist.gov";
static const char* NTP_SERVER_3 = "time.google.com";


/* =====================================================
   STATUS NTP
   ===================================================== */

static bool ntpInitialized = false;


/* =====================================================
   INIT NTP
   Dipanggil saat boot dan WiFi reconnect
   ===================================================== */

inline void initNTP() {

  /* ===================================================
     Jangan konfigurasi NTP jika WiFi belum terhubung
     =================================================== */

  if (WiFi.status() != WL_CONNECTED) {

    ntpInitialized = false;

    Serial.println(
      "[NTP] WiFi not connected"
    );

    return;
  }


  /* ===================================================
     Konfigurasi waktu
     =================================================== */

  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    NTP_SERVER_1,
    NTP_SERVER_2,
    NTP_SERVER_3
  );


  ntpInitialized = true;


  Serial.println(
    "[NTP] Time synchronization started"
  );

  Serial.print(
    "[NTP] Timezone: UTC+"
  );

  Serial.println(
    GMT_OFFSET_SEC / 3600
  );
}


/* =====================================================
   CEK APAKAH DATA WAKTU VALID
   ===================================================== */

inline bool isTimeValid(
  const struct tm* t
) {

  if (t == nullptr) {
    return false;
  }


  /*
     Tahun ESP32 biasanya dimulai dari 1970
     sebelum NTP tersinkron.

     Tahun 2020 digunakan sebagai batas aman.
  */

  if (t->tm_year < 120) {
    return false;
  }


  /*
     Validasi bulan
  */

  if (
    t->tm_mon < 0 ||
    t->tm_mon > 11
  ) {
    return false;
  }


  /*
     Validasi tanggal
  */

  if (
    t->tm_mday < 1 ||
    t->tm_mday > 31
  ) {
    return false;
  }


  return true;
}


/* =====================================================
   GET LOCAL TIME AMAN
   ===================================================== */

inline bool getLocalTimeSafe(
  struct tm* t
) {

  if (t == nullptr) {
    return false;
  }


  /*
     Jika WiFi tidak tersambung,
     jangan menunggu NTP.
  */

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }


  /*
     Timeout 200 ms.

     Lebih ringan daripada menunggu 1 detik.
  */

  if (!getLocalTime(t, 200)) {

    return false;
  }


  /*
     Pastikan waktu benar-benar valid.
  */

  if (!isTimeValid(t)) {

    return false;
  }


  return true;
}


/* =====================================================
   CEK STATUS NTP
   ===================================================== */

inline bool isNTPReady() {

  struct tm t;

  return getLocalTimeSafe(&t);
}


/* =====================================================
   GET JAM
   ===================================================== */

inline String getTimeString() {

  struct tm t;


  if (!getLocalTimeSafe(&t)) {

    return "--:--:--";
  }


  char buf[9];

  strftime(
    buf,
    sizeof(buf),
    "%H:%M:%S",
    &t
  );


  return String(buf);
}


/* =====================================================
   GET TANGGAL
   ===================================================== */

inline String getDateString() {

  struct tm t;


  if (!getLocalTimeSafe(&t)) {

    return "--/--/----";
  }


  char buf[11];

  strftime(
    buf,
    sizeof(buf),
    "%d/%m/%Y",
    &t
  );


  return String(buf);
}


/* =====================================================
   DEBUG PRINT WAKTU
   ===================================================== */

inline void printCurrentTime() {

  struct tm t;


  if (!getLocalTimeSafe(&t)) {

    Serial.println(
      "[NTP] Time not available"
    );

    return;
  }


  Serial.printf(
    "[NTP] %02d/%02d/%04d %02d:%02d:%02d\n",

    t.tm_mday,
    t.tm_mon + 1,
    t.tm_year + 1900,

    t.tm_hour,
    t.tm_min,
    t.tm_sec
  );
}


#endif