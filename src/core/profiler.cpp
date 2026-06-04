#include "profiler.h"

#ifdef DEBUG_PROFILER

#include <WiFi.h>

namespace {
constexpr uint8_t kMaxItems = 96;
constexpr uint32_t kReportIntervalMs = 10000;

struct ProfileItem {
    const char* name = nullptr;
    uint32_t count = 0;
    uint32_t lastUs = 0;
    uint32_t minUs = UINT32_MAX;
    uint32_t maxUs = 0;
    uint64_t totalUs = 0;
};

ProfileItem items[kMaxItems];
uint32_t lastReportMs = 0;
bool announced = false;
uint32_t droppedRecords = 0;

ProfileItem* findOrCreate(const char* name) {
    for (auto& item : items) {
        if (item.name && strcmp(item.name, name) == 0) { return &item; }
    }
    for (auto& item : items) {
        if (item.name == nullptr) {
            item.name = name;
            return &item;
        }
    }
    return nullptr;
}

void printItem(const ProfileItem& item) {
    if (!item.name || item.count == 0) { return; }
    const uint32_t avgUs = static_cast<uint32_t>(item.totalUs / item.count);
    Serial.printf("  %-16s count=%lu avg=%luus max=%luus last=%luus\n",
                  item.name,
                  static_cast<unsigned long>(item.count),
                  static_cast<unsigned long>(avgUs),
                  static_cast<unsigned long>(item.maxUs),
                  static_cast<unsigned long>(item.lastUs));
}
} // namespace

Profiler profiler;

ProfileScope::ProfileScope(const char* name) : _name(name), _start(micros()) {}

ProfileScope::~ProfileScope() {
    profiler.record(_name, micros() - _start);
}

void Profiler::record(const char* name, uint32_t elapsedUs) {
    ProfileItem* item = findOrCreate(name);
    if (!item) {
        droppedRecords++;
        return;
    }
    item->count++;
    item->lastUs = elapsedUs;
    if (elapsedUs < item->minUs) { item->minUs = elapsedUs; }
    if (elapsedUs > item->maxUs) { item->maxUs = elapsedUs; }
    item->totalUs += elapsedUs;
}

void Profiler::loop() {
    if (!announced && millis() > 1000) {
        announced = true;
        Serial.println("#PROF# DEBUG_PROFILER active, reporting every 10s");
    }
    report(false);
}

void Profiler::report(bool force) {
    const uint32_t now = millis();
    if (!force && now - lastReportMs < kReportIntervalMs) { return; }
    lastReportMs = now;

    Serial.println("#PROF# 10s");
    for (const auto& item : items) {
        printItem(item);
    }
    if (droppedRecords > 0) {
        Serial.printf("  profiler_dropped=%lu\n", static_cast<unsigned long>(droppedRecords));
    }
    Serial.printf("  heap_free=%lu heap_min=%lu psram_free=%lu wifi_rssi=%d\n",
                  static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getMinFreeHeap()),
                  static_cast<unsigned long>(ESP.getFreePsram()),
                  WiFi.RSSI());
    Serial.println("##PROF#");

    reset();
}

void Profiler::reset() {
    droppedRecords = 0;
    for (auto& item : items) {
        if (!item.name) { continue; }
        item.count = 0;
        item.lastUs = 0;
        item.minUs = UINT32_MAX;
        item.maxUs = 0;
        item.totalUs = 0;
    }
}

#endif
