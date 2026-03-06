#include "scheduler.h"

Scheduler scheduler;

// ════════════════════════════════════════════════
//  CONSTRUCTOR
// ════════════════════════════════════════════════
Scheduler::Scheduler() {
  enabled        = false;
  eventCount     = 0;
  _lastFiredHour = 255;
  _lastFiredMinute = 255;
  _lastFiredDay  = 255;
  memset(events, 0, sizeof(events));
}
// ════════════════════════════════════════════════
//  Time to Wake up
// ════════════════════════════════════════════════
uint32_t Scheduler::secondsToNext(uint8_t currentDay, uint8_t currentHour, uint8_t currentMinute) {
  if (!enabled || eventCount == 0) return 0;

  uint32_t nowMins  = currentDay * 1440 + currentHour * 60 + currentMinute;
  uint32_t bestDiff = 7 * 1440; // max tydzień w minutach

  for (uint8_t i = 0; i < eventCount; i++) {
    uint32_t eventMins = events[i].day * 1440 + events[i].hour * 60 + events[i].minute;
    uint32_t diff;
    if (eventMins > nowMins) {
      diff = eventMins - nowMins;
    } else {
      diff = 7 * 1440 - nowMins + eventMins; // następny tydzień
    }
    if (diff < bestDiff) bestDiff = diff;
  }

  return bestDiff * 60; // zwracamy sekundy
}


// ════════════════════════════════════════════════
//  LOAD z SPIFFS
// ════════════════════════════════════════════════
bool Scheduler::load() {
  if (!SPIFFS.exists(SCHEDULE_PATH)) {
    enabled    = false;
    eventCount = 0;
    return false;
  }
  File file = SPIFFS.open(SCHEDULE_PATH, "r");
  if (!file) return false;

  String content = file.readString();
  file.close();
	Serial.printf("Scheduler load() raw: %s\n", content.c_str());
	Serial.printf("Scheduler: loaded %d events, enabled=%d\n", eventCount, enabled);
	Serial.printf("Scheduler: load() - exists=%d\n", SPIFFS.exists(SCHEDULE_PATH));
	
  // reset
  enabled    = false;
  eventCount = 0;
  memset(events, 0, sizeof(events));

  // parsuj "enabled":X
  const char* enPos = strstr(content.c_str(), "\"enabled\":");
  if (enPos) {
    enabled = (atoi(enPos + 10) == 1);
  }

  // parsuj "sched":[{"d":X,"t":"HH:MM","a":"action"},...]
  const char* cursor = strstr(content.c_str(), "\"sched\":[");
  if (!cursor) return true;
  cursor += 9; // przesuń za "sched":[

  while (eventCount < SCHED_MAX_EVENTS) {
    // szukaj {"d":
    const char* dPos = strstr(cursor, "\"d\":");
    if (!dPos) break;

    // dzień
    uint8_t day = atoi(dPos + 4);

    // czas "t":"HH:MM"
    const char* tPos = strstr(dPos, "\"t\":\"");
    if (!tPos) break;
    tPos += 5;
    uint8_t hour   = atoi(tPos);
    uint8_t minute = atoi(tPos + 3);

    // akcja "a":"start/stop"
    const char* aPos = strstr(dPos, "\"a\":\"");
    if (!aPos) break;
    aPos += 5;
    char action[8] = {0};
    const char* aEnd = strchr(aPos, '"');
    if (!aEnd) break;
    strlcpy(action, aPos, aEnd - aPos + 1);

    // zapisz zdarzenie
    events[eventCount].day    = day;
    events[eventCount].hour   = hour;
    events[eventCount].minute = minute;
    strlcpy(events[eventCount].action, action, sizeof(events[eventCount].action));
    eventCount++;

    cursor = aEnd + 1;
	Serial.printf("Scheduler: loaded %d events, enabled=%d\n", eventCount, enabled);
	Serial.printf("Scheduler: load() - exists=%d\n", SPIFFS.exists(SCHEDULE_PATH));
  }

  return true;
}

// ════════════════════════════════════════════════
//  SAVE do SPIFFS
// ════════════════════════════════════════════════
bool Scheduler::save() {
	Serial.printf("Scheduler: save() - eventCount=%d\n", eventCount);
  File file = SPIFFS.open(SCHEDULE_PATH, "w");
  if (!file) return false;

  char* buf = (char*)malloc(2048);
  if (!buf) return false;
  
  buildJson(buf, 2048);
  file.print(buf);
  file.close();
  free(buf);
  return true;
}

// ════════════════════════════════════════════════
//  PARSOWANIE WARTOŚCI "dzień,HH:MM,akcja"
// ════════════════════════════════════════════════
bool Scheduler::_parseVal(const char* val, uint8_t& day, uint8_t& hour, uint8_t& minute, char* action) {
  // format: "3,15:42,start"
  char tmp[32] = {0};
  strlcpy(tmp, val, sizeof(tmp));

  char* p1 = strtok(tmp, ",");
  if (!p1) return false;
  day = atoi(p1);
  if (day > 6) return false;

  char* p2 = strtok(NULL, ",");
  if (!p2) return false;
  hour   = atoi(p2);
  minute = atoi(p2 + 3);
  if (hour > 23 || minute > 59) return false;

  char* p3 = strtok(NULL, ",");
  if (!p3) return false;
  if (strcmp(p3, "start") != 0 && strcmp(p3, "stop") != 0) return false;
  strlcpy(action, p3, 8);

  return true;
}

// ════════════════════════════════════════════════
//  ADD EVENT
// ════════════════════════════════════════════════
bool Scheduler::addEvent(const char* val) {
  if (eventCount >= SCHED_MAX_EVENTS) return false;

  uint8_t day, hour, minute;
  char action[8] = {0};
  if (!_parseVal(val, day, hour, minute, action)) return false;

  // sprawdź duplikat
  for (uint8_t i = 0; i < eventCount; i++) {
    if (events[i].day == day &&
        events[i].hour == hour &&
        events[i].minute == minute &&
        strcmp(events[i].action, action) == 0) {
      return true; // już istnieje
    }
  }

  events[eventCount].day    = day;
  events[eventCount].hour   = hour;
  events[eventCount].minute = minute;
  strlcpy(events[eventCount].action, action, sizeof(events[eventCount].action));
  eventCount++;

  return save();
}

// ════════════════════════════════════════════════
//  DELETE EVENT
// ════════════════════════════════════════════════
bool Scheduler::deleteEvent(const char* val) {
  uint8_t day, hour, minute;
  char action[8] = {0};
  if (!_parseVal(val, day, hour, minute, action)) return false;

  bool found = false;
  for (uint8_t i = 0; i < eventCount; i++) {
    if (events[i].day == day &&
        events[i].hour == hour &&
        events[i].minute == minute &&
        strcmp(events[i].action, action) == 0) {
      // przesuń pozostałe w lewo
      for (uint8_t j = i; j < eventCount - 1; j++) {
        events[j] = events[j + 1];
      }
      eventCount--;
      found = true;
      break;
    }
  }

  if (!found) return false;
  return save();
}

// ════════════════════════════════════════════════
//  SET ENABLED
// ════════════════════════════════════════════════
void Scheduler::setEnabled(bool en) {
  enabled = en;
  save();
}

// ════════════════════════════════════════════════
//  CHECK — wywołuj co minutę z głównej pętli
//  przekaż aktualny dzień tygodnia (0=Pn…6=Nd),
//  godzinę i minutę pobrane z NTP
// ════════════════════════════════════════════════
const char* Scheduler::check(uint8_t day, uint8_t hour, uint8_t minute) {
  if (!enabled) return "";

  // zabezpieczenie przed wielokrotnym odpaleniem tej samej minuty
  if (_lastFiredDay == day && _lastFiredHour == hour && _lastFiredMinute == minute) {
    return "";
  }

  for (uint8_t i = 0; i < eventCount; i++) {
    if (events[i].day == day &&
        events[i].hour == hour &&
        events[i].minute == minute) {
      _lastFiredDay    = day;
      _lastFiredHour   = hour;
      _lastFiredMinute = minute;
      return events[i].action; // "start" lub "stop"
    }
  }

  return "";
}

// ════════════════════════════════════════════════
//  BUILD JSON
// ════════════════════════════════════════════════
void Scheduler::buildJson(char* buf, size_t bufSize) {
  // {"sched":[{"d":0,"t":"07:00","a":"start"},...],"enabled":1}
  size_t pos = 0;
  pos += snprintf(buf + pos, bufSize - pos, "{\"sched\":[");

  for (uint8_t i = 0; i < eventCount; i++) {
    if (i > 0) pos += snprintf(buf + pos, bufSize - pos, ",");
    pos += snprintf(buf + pos, bufSize - pos,
      "{\"d\":%d,\"t\":\"%02d:%02d\",\"a\":\"%s\"}",
      events[i].day, events[i].hour, events[i].minute, events[i].action
    );
  }

  pos += snprintf(buf + pos, bufSize - pos,
    "],\"enabled\":%d}", enabled ? 1 : 0
  );
}

// ════════════════════════════════════════════════
//  BUILD OK JSON
// ════════════════════════════════════════════════
void Scheduler::buildOkJson(char* buf, size_t bufSize) {
  snprintf(buf, bufSize, "{\"sched_ok\":1}");
}
