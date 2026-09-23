#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "Time_NTP.h"
#include "relay_ctrl.h"
#include "sinric_ctrl.h"
#include "SinricProSwitch.h"

/* =====================================================
   PENANDA HARI TERAKHIR AUTO-OFF
   ===================================================== */

static int lastOffYear = -1;
static int lastOffYDay = -1;


/* =====================================================
   AUTO OFF SEMUA KIPAS PADA TENGAH MALAM
   ===================================================== */
inline void handleAutoOffAtMidnight(const char* switchID[]) {

  struct tm t;

  // Jika waktu NTP belum tersedia, jangan jalankan scheduler
  if (!getLocalTimeSafe(&t)) {
    return;
  }


  /* ===================================================
     IDENTITAS HARI

     tm_year = tahun sejak 1900
     tm_yday  = hari ke-berapa dalam tahun (0-365)
     =================================================== */

  int currentYear = t.tm_year;
  int currentYDay = t.tm_yday;


  /* ===================================================
     CEK APAKAH AUTO-OFF HARI INI SUDAH DIJALANKAN
     =================================================== */

  if (
    currentYear == lastOffYear &&
    currentYDay == lastOffYDay
  ) {
    return;
  }


  /* ===================================================
     CEK WAKTU TENGAH MALAM

     00:00 sampai sebelum 00:01
     =================================================== */

  if (
    t.tm_hour == 0 &&
    t.tm_min == 0
  ) {

    Serial.println(
      "[SCHEDULER] Auto OFF all fans"
    );


    /* ================================================
       MATIKAN SEMUA RELAY
       ================================================ */

    for (int i = 0; i < RELAY_COUNT; i++) {

      setRelay(
        i,
        false
      );
    }


    /* ================================================
       SINKRONISASI STATE KE SINRIC
       ================================================ */

    for (int i = 0; i < RELAY_COUNT; i++) {

      SinricProSwitch &sw =
        SinricPro[switchID[i]];

      sw.sendPowerStateEvent(false);
    }


    /* ================================================
       SIMPAN HARI YANG SUDAH DIPROSES
       ================================================ */

    lastOffYear = currentYear;
    lastOffYDay = currentYDay;


    Serial.print(
      "[SCHEDULER] Processed year="
    );

    Serial.print(
      currentYear + 1900
    );

    Serial.print(
      " yday="
    );

    Serial.println(
      currentYDay
    );
  }
}

#endif