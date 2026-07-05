
#define VERSION v8.8_TB
// clang-format off
/*
   Read the before use !!!
   https://github.com/tomaszbunio/yoRadio_TB/blob/main/README.md

  - The // sign at the beginning of the line makes the command inactive, so the compiler ignores it! 
    This allows you to set the appropriate configuration for your hardware.
*/

#ifndef myoptions_h
#define myoptions_h

#ifndef ARDUINO_ESP32S3_DEV
    #define ARDUINO_ESP32S3_DEV
#endif

//#define SCREEN 4

// #define HEAP_DBG
// #define DEBUG_PROFILER       // Lekki profiler: display.loop/draw, web, main, player/audio, network
// #define DEBUG_MODE_SWITCH   // Debug przełączania trybów (IR MENU / playlist return / display mode)

/* You can set the program language here.
   Supported languages: HU NL PL RU EN GR SK DE UA ES. */
#define L10N_LANGUAGE PL

/* Display name days --
Supported languages: HU, PL, NL, GR, DE (UA Local/namedays/namedays_UA.h is not filled in.) */
#define NAMEDAYS_FILE PL

#define USE_BUILTIN_LED false /* The RGB LED does not turn on.. */

/* DspTask na Core 1 – AudioTask na Core 0 */
#define DSP_TASK_CORE_ID 1

/* netserver.loop() w Arduino loop() (Core 1), bezwarunkowo – bez bramkowania przez timekeeper.
   Wymagane aby shouldReboot działał przy DSP_TASK_CORE_ID 1. */
#define NETSERVER_LOOP1

/* Arduino OTA Support */
#define USE_OTA true                    /* Enable OTA updates from Arduino IDE */
// #define OTA_PASS "myotapassword12345"   /* OTA password for secure updates */

/* HTTP Authentication */
// #define HTTP_USER ""               /* HTTP basic authentication username */
// #define HTTP_PASS ""               /* HTTP basic authentication password */

/*----- LCD DISPLAY -----*/
#define DSP_MODEL DSP_ILI9488
// #define DSP_MODEL DSP_ILI9341
// #define DSP_MODEL DSP_ST7796

/*----- OLED DISPLAY -----*/
// #define DSP_MODEL      DSP_SSD1322

/*----- DISPLAY PIN SETS -----*/
#define TFT_DC         9
#define TFT_CS         10
#define TFT_RST        -1
#define BRIGHTNESS_PIN 21
/*
   GPIO 11 - MOSI
   GPIO 12 - CLK
   GPIO 13 - MISO  // - Do not connect to the LCD display!!!
*/

/*----- Touch ISP -----*/
 #define ENABLE_TOUCH
 #define TS_MODEL TS_MODEL_XPT2046
 #define TS_CS    48
 
/*----- Touch I2C -----*/
// #define TS_MODEL TS_MODEL_FT6X36
// #define TS_SCL      7
// #define TS_SDA      8
// #define TS_INT     17 

/*----- Touch corrections -----*/
 //#define X_TOUCH_MIRRORING
 //#define Y_TOUCH_MIRRORING

/*----- NEXTION DISPLAY serial port -----*/
// #define NEXTION_RX			15
// #define NEXTION_TX			16

/* PCM5102A  DAC */
#define I2S_DOUT    16
#define I2S_BCLK    17
#define I2S_LRC     15

/* ENCODER 1 */
#define ENC_BTNR      4  // S2
#define ENC_BTNL      5  // S1
#define ENC_BTNB      6  // KEY
//#define ENC_INTERNALPULLUP	true

/* ENCODER 2 */
#define ENC2_BTNR      255  // S2
#define ENC2_BTNL      255  // S1
#define ENC2_BTNB      255  // KEY
#define ENC2_INTERNALPULLUP	true

// Dwa niezależne enkodery: lewy=stacje, prawy=głośność
#define TWO_ENCODERS

/*----- CLOCK MODUL RTC DS3132 -----*/
// #define RTC_SCL			     7
// #define RTC_SDA			     8
// #define RTC_MODULE DS3231

/*----- REMOTE CONTROL INFRARED RECEIVER -----*/
#define IR_PIN 2

/* Built-in IR default mapping (loaded when IR EEPROM section is empty). */
#include "myir_defaults.h"

/*----- SD CARD -----*/
 #define SDC_CS     38
 //#define SDSPISPEED 4000000 /* 4MHz - Slower speed to prevent display flicker on shared SPI bus */

 /*----- by Maciej Bednarski -----*/
/*---- Activating this will move the cursor up and down in the playlist -----*/
#define PLAYLIST_SCROLL_MOVING_CURSOR

/*----- The color display displays a grayscale image -----*/
// #define THEME_GRAY

// niebieskie podświetlenie playlisty
#define PL_WIDGET_WITCH_BAR true  // niebieskie tło na playliście

/*----- Inactive segments of the clock, true or false. -----*/
 //#define CLOCKFONT_MONO

/* Define 12-hour time format. -----*/
// #define AM_PM_STYLE

/*-----  Turn on the original seven-segment font. -----*/
 #define CLOCKFONT VT_DIGI_OLD

/*----- Speaks the time using Google TTS voice in the specified language and every specified minute. -----*/
#define CLOCK_TTS_ENABLED          true // Enabled (true) or disabled (false)
#define CLOCK_TTS_LANGUAGE         "PL" // Language ( EN, HU, PL, NL, DE, RU, RO ,FR, GR)
#define CLOCK_TTS_INTERVAL_MINUTES 15   // Hány percenként mondja be. - How many times a minute does it say.

// Ukryj widget pogody – zajmuje obszar nakładający się na flip clock
//#define HIDE_WEATHER

/*----- With this setting there is no scrolling on the weather bar. -----*/
//#define WEATHER_FMT_SHORT

/*----- With this setting, the full weather report is displayed. -----*/
 #define EXT_WEATHER  true

/*----- With this setting, the wind speed will be km/h. -----*/
 //#define WIND_SPEED_IN_KMH

/* The VU meter supports two types of display modes.
BOOMBOX_STYLE is the style when the display swings out from the center to two sides. You can set it here.
If there is a // sign at the beginning of the line, the basic VU meter is working, swinging out from left to right. -----*/
 #define BOOMBOX_STYLE

/*----- A white bar will appear at the end of the VU meter at the peak values ​​if you enable this. -----*/
#define VU_PEEK

 /* When selecting from the station list, you do not need to press the rotary encoder button, the channel will automatically
change when you exit. (By Zsigmond Becskehazi) -----*/
#define DIRECT_CHANNEL_CHANGE

/*----- How long to return to the main screen from the station list. (seconds) -----*/
#define STATIONS_LIST_RETURN_TIME 3

/*-----This pin controls the amplifier's power supply. When music is playing, the pin is set to HIGH to control the relay.
When music is not playing (stopped or volume is 0), the pin is set to LOW. This change occurs when the screensaver is running. -----*/
// #define PWR_AMP 2

// #define WAKE_PIN	42

/*----- by Andrzej Jaroszuk -----*/    
/*----- Stops playback in internet radio mode when the playback buffer runs out. Then restarts playback. -----*/
#define ENABLE_STALL_WATCHDOG

/*----- Read station name only from playlist ----- */
#define METADATA_STATION_NAME_OFF

/*------Neopixel-----------------------------------------*/

#define NEOPIXEL_ON
#ifdef NEOPIXEL_ON
   #define NEOPIXEL_PIN 42
   #define LED_COUNT    16
#endif

/*----- Tested on Synology NAS ----- */
// #define USE_DLNA
// #define dlnaHost "192.168.1.200"
// #define dlnaIDX  21

// zegar w stylu FLIP CLOCK
#define FLIP_DIGIT_PAD  1   // padding wewnątrz panelu (px)
#define FLIP_DIGIT_GAP  1   // odstęp między panelami (px)
#define FLIP_PANEL_VPAD 10   // margines pionowy: 5px góra + 5px dół panelu

/*----- Automatyczna zmiana czasu letniego/zimowego (DST) -----*/
/*----- Automatic DST adjustment via POSIX TZ string          -----*/
/* Wybierz/Select TIMEZONE_POSIX odpowiednie dla swojej lokalizacji: */
/*                                                                    */
/* DE  Niemcy       (Germany):     "CET-1CEST,M3.5.0,M10.5.0/3"    */
/* EN  Wielka Bryt. (UK):          "GMT0BST,M3.5.0/1,M10.5.0"       */
/* ES  Hiszpania    (Spain):       "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* GR  Grecja       (Greece):      "EET-2EEST,M3.5.0/3,M10.5.0/4"   */
/* HU  Węgry        (Hungary):     "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* IT  Włochy       (Italy):       "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* NL  Holandia     (Netherlands): "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* PL  Polska       (Poland):      "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* RU  Rosja/Moskwa (Russia/MSK):  "MSK-3"                           */
/* SK  Słowacja     (Slovakia):    "CET-1CEST,M3.5.0,M10.5.0/3"     */
/* UA  Ukraina      (Ukraine):     "EET-2EEST,M3.5.0/3,M10.5.0/4"   */
/*                                                                    */
/* Pełna baza stref: https://github.com/nayarsystems/posix_tz_db     */
#define TIMEZONE_POSIX "CET-1CEST,M3.5.0,M10.5.0/3"

/*----- Deep sleep wake sources -----*/
// false = wybudzanie tylko przyciskiem enkodera (stabilniej)
// true  = dodatkowo wybudzanie przez IR (może łapać zakłócenia)
#define SLEEP_WAKE_BY_IR false

/*----- Autostart przy każdym włączeniu (0=idle, 1=ostatni stan, 2=zawsze) -----*/
//#define SMARTSTART_DEFAULT 2

/*----- Wznowienie odtwarzania SD od ostatniego miejsca po zatrzymaniu -----*/
#define SD_RESUME_ENABLED

/*----- Zawsze zacznij odtwarzanie SD od 1. utworu (ignoruj zapamiętany numer) -----*/
//#define SD_ALWAYS_START_FROM_FIRST

/*----- Widget logo stacji radiowej -----*/
/*  Pliki PNG (160x120, 24-bit) umieszczaj na SPIFFS w katalogu głównym.
    Nazwa pliku = nazwa stacji ze spacjami zamienionymi na '_', np.:
      "Eska Rock"  →  /Eska_Rock.png
      "RMF FM"     →  /RMF_FM.png
    Plik domyślny (gdy brak logo stacji): /logo_default.png
    Wgrywanie: PlatformIO → Upload Filesystem Image, www → Settings/Board              */
#define STATION_LOGO_WIDGET
#ifdef STATION_LOGO_WIDGET
  #define STATION_LOGO_X   1   // pozycja X lewego boku widgetu (px)
  #define STATION_LOGO_Y   104  // pozycja Y górnego boku widgetu (px)
  #define STATION_LOGO_W  120
  #define STATION_LOGO_H   90

  // Fallback logo: gdy brak pliku dla stacji, używa logo nadawcy nadrzędnego.
  // Format: { "prefiks_stacji", "plik_fallback" } – obie wartości lowercase, bez .raw
  // Prefiks porównywany jest z początkiem znormalizowanej nazwy stacji.
  #define STATION_LOGO_FALLBACKS \
    { "rmf",        "rmf_fm"       }, \
    { "zet",        "radio_zet"    }, \
    { "antyradio",  "antyradio"    }, \
    { "eska",       "radio_eska"   }, \
    { "open fm",    "open_fm"      }, \
    { "e m",        "radio_em"      },
#endif

/*----- Okładka albumu na ekranie SD_PLAYER -----*/
//#define SD_COVER_ART
#ifdef SD_COVER_ART
  #define SD_COVER_W  215
  #define SD_COVER_H  215
  #define SD_COVER_X  10   // TFT_FRAMEWDT
  #define SD_COVER_Y  95   // dół okładki na dolnej linii paska postępu
  // Odkomentuj, aby pobierać okładki z Last.fm. Zakomentowane = szuka lokalnego front.jpg.
  #define SD_COVER_SOURCE_LASTFM

  #ifdef SD_COVER_SOURCE_LASTFM
    #define LASTFM_API_KEY "0b783abd18c1a0b35c2261b4d5a7a046"
    #define LASTFM_COVER_TIMEOUT_MS 5000
  #endif
  #define CORE_STACK_SIZE (1024 * 16)  // JPEGDEC decode needs larger DspTask stack
#endif

#endif // myoptions_h
