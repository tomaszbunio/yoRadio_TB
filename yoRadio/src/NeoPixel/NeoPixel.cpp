#include <Adafruit_NeoPixel.h>
#include "NeoPixel.h"
#include "../core/controls.h"
#include "../core/network.h"
#include "../core/display.h"
#include "myoptions.h"
// ------------------------------------------------------
// KONFIGURACJA SPRZĘTOWA
// ------------------------------------------------------

Adafruit_NeoPixel strip(LED_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ------------------------------------------------------
// ZMIENNE GLOBALNE
// ------------------------------------------------------
static LedState currentLedState = LED_BOOT;
static uint8_t currentEffect = 0;
static uint32_t lastUpdate = 0;

// ------------------------------------------------------
// COLOR WIPE
// ------------------------------------------------------
static uint16_t wipeIndex = 0;
static uint32_t wipeColor = 0;
static uint16_t wipeWait = 0;
static bool wipeActive = false;

// ------------------------------------------------------
// RAINBOW
// ------------------------------------------------------
static uint16_t rainbowIndex = 0;
static uint16_t rainbowWait = 0;
static bool rainbowActive = false;

// ------------------------------------------------------
// RAINBOW CYCLE
// ------------------------------------------------------
static uint16_t rainbowCycleIndex = 0;
static uint16_t rainbowCycleWait = 0;
static bool rainbowCycleActive = false;

// ------------------------------------------------------
// THEATER CHASE
// ------------------------------------------------------
static uint32_t chaseColor = 0;
static uint16_t chaseWait = 0;
static uint8_t chaseQ = 0;
static uint8_t chaseJ = 0;
static bool chaseActive = false;

// ------------------------------------------------------
// ENCODER OVERLAY
// ------------------------------------------------------
static bool encoderAnimActive = false;
static int encoderPos = 0;
static uint32_t encoderLast = 0;

// ------------------------------------------------------
// NARZĘDZIA
// ------------------------------------------------------
static void stopAllEffects() {
  wipeActive = rainbowActive = rainbowCycleActive = false;
  chaseActive = false;
  strip.clear();
  strip.show();
}

static uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// ------------------------------------------------------
// STARTY EFEKTÓW
// ------------------------------------------------------
static void startColorWipe(uint32_t color, uint16_t wait) {
  wipeIndex = 0;
  wipeColor = color;
  wipeWait = wait;
  wipeActive = true;
}

static void startRainbowCycle(uint16_t wait) {
  rainbowCycleIndex = 0;
  rainbowCycleWait = wait;
  rainbowCycleActive = true;
}

static void startTheaterChase(uint32_t color, uint16_t wait) {
  chaseColor = color;
  chaseWait = wait;
  chaseQ = 0;
  chaseJ = 0;
  chaseActive = true;
}

// ------------------------------------------------------
// KROKI EFEKTÓW
// ------------------------------------------------------
static void colorWipe_step() {
  if (!wipeActive || millis() - lastUpdate < wipeWait) return;
  lastUpdate = millis();

  strip.setPixelColor(wipeIndex++, wipeColor);
  strip.show();

  if (wipeIndex >= strip.numPixels()) wipeActive = false;
}

static void rainbowCycle_step() {
  if (!rainbowCycleActive || millis() - lastUpdate < rainbowCycleWait) return;
  lastUpdate = millis();

  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbowCycleIndex) & 255));
  }
  strip.show();

  rainbowCycleIndex = (rainbowCycleIndex + 1) % (256 * 5);
}

static void theaterChase_step() {
  if (!chaseActive || millis() - lastUpdate < chaseWait) return;
  lastUpdate = millis();

  for (uint16_t i = 0; i < strip.numPixels(); i += 3)
    strip.setPixelColor(i + chaseQ, chaseColor);

  strip.show();

  for (uint16_t i = 0; i < strip.numPixels(); i += 3)
    strip.setPixelColor(i + chaseQ, 0);

  chaseQ = (chaseQ + 1) % 3;
}

// ------------------------------------------------------
// STEROWANIE STANEM LED
// ------------------------------------------------------
 void setLedState(LedState state) {
  if (state == currentLedState) return;
  currentLedState = state;

  stopAllEffects();

  switch (state) {
    case LED_PLAYER:
      startColorWipe(strip.Color(0, 0, 80), 20);
      currentEffect = 1;
      break;

    case LED_STATIONS:
      startTheaterChase(strip.Color(0, 80, 0), 60);
      currentEffect = 4;
      break;

    case LED_SCREENSAVER:
      startRainbowCycle(40);
      currentEffect = 3;
      break;

    case LED_LOST:
	  startTheaterChase(strip.Color(80, 0, 0), 120);
      currentEffect = 4;
      break;
    case LED_NO_WIFI:
      startTheaterChase(strip.Color(80, 0, 0), 120);
      currentEffect = 4;
      break;

    case LED_SD_READY:
      startColorWipe(strip.Color(80, 0, 80), 30);
      currentEffect = 1;
      break;

    default:
      break;
  }
}

// ------------------------------------------------------
// DETEKCJA STANU SYSTEMU (YO-RADIO)
// ------------------------------------------------------
void updateLedState() {
  if (network.status != CONNECTED) {
    setLedState(LED_NO_WIFI);
    return;
  }

  if (network.status == SDREADY) {
    setLedState(LED_SD_READY);
    return;
  }

  switch (display.mode()) {
    case PLAYER:      setLedState(LED_PLAYER); break;
    case STATIONS:    setLedState(LED_STATIONS); break;
    case SCREENSAVER: setLedState(LED_SCREENSAVER); break;
    case LOST:        setLedState(LED_LOST); break;
    default: break;
  }
}


void NeoPixel_off() {
  strip.clear();
  strip.show();
}

// ------------------------------------------------------
// OVERLAY: ENKODER
// ------------------------------------------------------
void ledEncoderRotate(bool dir) {
  encoderAnimActive = true;
  encoderLast = millis();

  encoderPos += dir ? 1 : -1;
  if (encoderPos < 0) encoderPos = strip.numPixels() - 1;
  if (encoderPos >= strip.numPixels()) encoderPos = 0;
}

static void encoderOverlayStep() {
  if (!encoderAnimActive) return;
  if (millis() - encoderLast > 120) {
    encoderAnimActive = false;
    return;
  }
  strip.setPixelColor(encoderPos, strip.Color(80, 80, 80));
  strip.show();
}

// ------------------------------------------------------
// INIT + LOOP
// ------------------------------------------------------
void NeoPixel_init() {
  strip.begin();
  strip.clear();
  strip.show();
  setLedState(LED_BOOT);
}

void NeoPixel_loop() {
	updateLedState();
  switch (currentEffect) {
    case 1: colorWipe_step(); break;
    case 3: rainbowCycle_step(); break;
    case 4: theaterChase_step(); break;
    default: break;
  }
  encoderOverlayStep();
}