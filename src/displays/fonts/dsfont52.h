#ifndef dsfont_h
#define dsfont_h
#include "../../core/options.h"

#if CLOCKFONT == VT_DIGI_OLD
  #ifdef AM_PM_STYLE
    #include "VT_DIGI_OLD_20x11s.h"
    #include "VT_DIGI_OLD_47x26.h"
  #else
    #include "VT_DIGI_OLD_27x15s.h"
    #include "VT_DIGI_OLD_47x26.h"
  #endif
#else  // CLOCKFONT == VT_DIGI
  #ifdef AM_PM_STYLE
    #include "VT_DIGI_20x11s.h"  // A másodperc fontja
    #include "VT_DIGI_47x26.h"
  #else
    #include "VT_DIGI_27x15s.h"  // A másodperc fontja
    #include "VT_DIGI_47x26.h"
  #endif
#endif

// The 320x240 layout has one compile-time selected 47 px font pair.
// Keep the runtime clock-font API used by widgets.cpp, falling back to that
// pair when an EEPROM value selects a font available only in dsfont70.h.
static inline uint8_t yoClockFontSanitize(uint8_t id) {
  return id == (uint8_t)CLOCKFONT ? id : (uint8_t)CLOCKFONT;
}

static inline const GFXfont* yoClockFontMain(uint8_t id) {
  (void)id;
  return &Clock_GFXfont;
}

static inline const GFXfont* yoClockFontSec(uint8_t id) {
  (void)id;
  return &Clock_GFXfont_sec;
}

#endif
