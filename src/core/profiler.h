#ifndef profiler_h
#define profiler_h

#include "options.h"

#ifdef DEBUG_PROFILER

#include <Arduino.h>

class ProfileScope {
  public:
    explicit ProfileScope(const char* name);
    ~ProfileScope();

  private:
    const char* _name;
    uint32_t _start;
};

class Profiler {
  public:
    void record(const char* name, uint32_t elapsedUs);
    void loop();
    void report(bool force = false);
    void reset();
};

extern Profiler profiler;

#define PROFILE_SCOPE(name) ProfileScope _profile_scope_##__LINE__(name)
#define PROFILE_TIMER_START(timer) uint32_t timer = micros()
#define PROFILE_TIMER_RECORD(name, timer) profiler.record(name, micros() - timer)
#define PROFILE_LOOP() profiler.loop()
#define PROFILE_REPORT() profiler.report(true)

#else

#define PROFILE_SCOPE(name) do { } while (0)
#define PROFILE_TIMER_START(timer) do { } while (0)
#define PROFILE_TIMER_RECORD(name, timer) do { } while (0)
#define PROFILE_LOOP() do { } while (0)
#define PROFILE_REPORT() do { } while (0)

#endif

#endif
