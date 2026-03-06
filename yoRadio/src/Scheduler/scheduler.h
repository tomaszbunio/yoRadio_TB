#pragma once
#include <Arduino.h>
#include <SPIFFS.h>

#define SCHEDULE_PATH "/www/schedule.json"
#define SCHED_MAX_EVENTS 50  // maksymalna liczba zdarzeń

struct sched_event_t {
  uint8_t  day;     // 0=Pon, 1=Wt, ... 6=Ndz
  uint8_t  hour;
  uint8_t  minute;
  char     action[8]; // "start" lub "stop"
};

class Scheduler {
public:
  bool    enabled;
  uint8_t eventCount;
  sched_event_t events[SCHED_MAX_EVENTS];
  uint32_t secondsToNext(uint8_t currentDay, uint8_t currentHour, uint8_t currentMinute);
  Scheduler();

  // Zapis/odczyt z SPIFFS
  bool load();
  bool save();

  // Parsowanie komend WS od frontendu
  // sched_save=3,15:42,start
  bool addEvent(const char* val);
  // sched_del=3,15:42,start
  bool deleteEvent(const char* val);
  // sched_enable=1
  void setEnabled(bool en);

  // Sprawdzanie czy teraz trzeba coś uruchomić
  // zwraca "start", "stop" lub "" jeśli nic
  const char* check(uint8_t day, uint8_t hour, uint8_t minute);

  // Budowanie JSON do wysłania do frontendu
  // {"sched":[{"d":0,"t":"07:00","a":"start"},...], "enabled":1}
  void buildJson(char* buf, size_t bufSize);

  // Budowanie potwierdzenia
  // {"sched_ok":1}
  static void buildOkJson(char* buf, size_t bufSize);

private:
  uint8_t  _lastFiredHour;
  uint8_t  _lastFiredMinute;
  uint8_t  _lastFiredDay;
  bool     _parseVal(const char* val, uint8_t& day, uint8_t& hour, uint8_t& minute, char* action);
};

extern Scheduler scheduler;
