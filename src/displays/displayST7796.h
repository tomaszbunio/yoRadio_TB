#ifndef displayST7796_h
#define displayST7796_h

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include "../Adafruit_ST7796S/Adafruit_ST7796S_kbv.h"
#include "fonts/bootlogo80x80.h"
#include "fonts/dsfont70.h"

typedef GFXcanvas16 Canvas;
typedef Adafruit_ST7796S_kbv yoDisplay;

#ifndef STATION_LOGO_W
  #define STATION_LOGO_W 120
#endif
#ifndef STATION_LOGO_H
  #define STATION_LOGO_H 90
#endif

#include "tools/commongfx.h"

#if __has_include("conf/displayST7796conf_custom.h")
  #include "conf/displayST7796conf_custom.h"
#else
  #include "conf/displayST7796conf.h"
#endif

#endif
