// Runtime clock fonts (320x480 / TIME_SIZE=70) - ILI9488 3.5"
// Enables runtime selection of clock font (WEB -> EEPROM).
#ifndef dsfont_h
#define dsfont_h
#include "../../core/options.h"

// ---------------- VT_DIGI_OLD ----------------
#define Clock_GFXfont Clock_VT_DIGI_OLD_main
#include "VT_DIGI_OLD_70x42.h"
#undef Clock_GFXfont

#ifdef AM_PM_STYLE
  #define Clock_GFXfont_sec Clock_VT_DIGI_OLD_sec
  #include "VT_DIGI_OLD_27x15s.h"
  #undef Clock_GFXfont_sec
#else
  #define Clock_GFXfont_sec Clock_VT_DIGI_OLD_sec
  #include "VT_DIGI_OLD_35x21s.h"
  #undef Clock_GFXfont_sec
#endif

// ---------------- VT_DIGI ----------------
#define Clock_GFXfont Clock_VT_DIGI_main
#include "VT_DIGI_68x38.h"
#undef Clock_GFXfont

#ifdef AM_PM_STYLE
  #define Clock_GFXfont_sec Clock_VT_DIGI_sec
  #include "VT_DIGI_27x15s.h"
  #undef Clock_GFXfont_sec
#else
  #define Clock_GFXfont_sec Clock_VT_DIGI_sec
  #include "VT_DIGI_34x19s.h"
  #undef Clock_GFXfont_sec
#endif

// ---------------- ARIMO ----------------
#include "Arimo_Regular_72.h"
#include "Arimo_Regular_36.h"

// ---------------- LARADOTSERIF ----------------
#define Clock_GFXfont Clock_LARADOTSERIF_sec
#include "LaradotSerif25pt7b_mono.h"
#undef Clock_GFXfont
#define Clock_GFXfont Clock_LARADOTSERIF_main
#include "LaradotSerif50pt7b_mono.h"
#undef Clock_GFXfont

// ---------------- OFFICE ----------------
#define Clock_GFXfont Clock_OFFICE_sec
#include "Office26pt7b_mono.h"
#undef Clock_GFXfont
#define Clock_GFXfont Clock_OFFICE_main
#include "Office52pt7b_mono.h"
#undef Clock_GFXfont

// ---------------- OLDTIMER ----------------
#define Clock_GFXfont Clock_OLDTIMER_sec
#include "Oldtimer20pt7b_mono.h"
#undef Clock_GFXfont
#define Clock_GFXfont Clock_OLDTIMER_main
#include "Oldtimer41pt7b_mono.h"
#undef Clock_GFXfont

// ---------------- SQUAREFONT ----------------
#define Clock_GFXfont Clock_SQUARE_sec
#include "SquareFont25pt7b_mono.h"
#undef Clock_GFXfont
#define Clock_GFXfont Clock_SQUARE_main
#include "SquareFont48pt7b_mono.h"
#undef Clock_GFXfont

// ---------------- POINTEDLYMAD ----------------
#define Clock_GFXfont Clock_POINTEDLYMAD_sec
#include "PointedlyMad25pt7b_mono.h"
#undef Clock_GFXfont
#define Clock_GFXfont Clock_POINTEDLYMAD_main
#include "PointedlyMad51pt7b_mono.h"
#undef Clock_GFXfont

static inline uint8_t yoClockFontSanitize(uint8_t id){
  if(id < VT_DIGI || id > POINTEDLYMAD_51) return (uint8_t)CLOCKFONT;
  return id;
}

static inline const GFXfont* yoClockFontMain(uint8_t id){
  id = yoClockFontSanitize(id);
  switch(id){
    case VT_DIGI_OLD:       return &Clock_VT_DIGI_OLD_main;
    case VT_DIGI:           return &Clock_VT_DIGI_main;
    case ARIMO_72:          return &Arimo_Regular_72;
    case LARADOTSERIF_50:   return &Clock_LARADOTSERIF_main;
    case OFFICE_52:         return &Clock_OFFICE_main;
    case OLDTIMER_41:       return &Clock_OLDTIMER_main;
    case SQUAREFONT_48:     return &Clock_SQUARE_main;
    case POINTEDLYMAD_51:   return &Clock_POINTEDLYMAD_main;
    default:                return &Clock_VT_DIGI_main;
  }
}

static inline const GFXfont* yoClockFontSec(uint8_t id){
  id = yoClockFontSanitize(id);
  switch(id){
    case VT_DIGI_OLD:       return &Clock_VT_DIGI_OLD_sec;
    case VT_DIGI:           return &Clock_VT_DIGI_sec;
    case ARIMO_72:          return &Arimo_Regular_36;
    case LARADOTSERIF_50:   return &Clock_LARADOTSERIF_sec;
    case OFFICE_52:         return &Clock_OFFICE_sec;
    case OLDTIMER_41:       return &Clock_OLDTIMER_sec;
    case SQUAREFONT_48:     return &Clock_SQUARE_sec;
    case POINTEDLYMAD_51:   return &Clock_POINTEDLYMAD_sec;
    default:                return &Clock_VT_DIGI_sec;
  }
}

#endif
