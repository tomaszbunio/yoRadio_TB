#include <Adafruit_NeoPixel.h>
#include "NeoPixel.h"
#include "../core/config.h"
#include "../core/display.h"
#include "../../myoptions.h"

Adafruit_NeoPixel strip(LED_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
static constexpr uint16_t RING1_START = 0;
static constexpr uint16_t RING_SIZE = 8;
static constexpr uint16_t RING2_START = RING1_START + RING_SIZE;

static uint8_t ringPhase1 = 0;
static uint8_t ringPhase2 = 0;
static uint32_t lastFrame = 0;

struct EncoderOverlay {
  bool active;
  int pos;
  uint32_t last;
  displayMode_e targetMode;
  bool modeSeen;
};

static EncoderOverlay encoderOverlay[2] = {
  {false, 0, 0, STATIONS, false},
  {false, 0, 0, VOL, false}
};

static uint32_t Wheel(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  if (wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

static uint32_t color565ToNeo(uint16_t color565) {
  uint8_t r = ((color565 >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color565 >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color565 & 0x1F) * 255 / 31;
  return strip.Color(r, g, b);
}

static uint32_t color565ToNeoScaled(uint16_t color565, uint8_t scale) {
  uint8_t r = (((color565 >> 11) & 0x1F) * 255 / 31) * scale / 255;
  uint8_t g = (((color565 >> 5) & 0x3F) * 255 / 63) * scale / 255;
  uint8_t b = ((color565 & 0x1F) * 255 / 31) * scale / 255;
  return strip.Color(r, g, b);
}

static void renderRing(uint16_t start, uint16_t end, uint8_t effect, uint16_t color565, uint8_t phase) {
  uint16_t count = end - start;
  if (count == 0) return;

  switch (effect) {
    case 0: // off
      for (uint16_t i = start; i < end; i++) strip.setPixelColor(i, 0);
      break;
    case 2: // theater chase
      for (uint16_t i = 0; i < count; i++) {
        strip.setPixelColor(start + i, ((i + phase) % 3 == 0) ? color565ToNeo(color565) : 0);
      }
      break;
    case 3: // rainbow cycle
      for (uint16_t i = 0; i < count; i++) {
        strip.setPixelColor(start + i, Wheel(((i * 256 / count) + phase) & 255));
      }
      break;
    case 4: { // breathing
      uint8_t breathPhase = phase * 2;
      uint8_t breath = (breathPhase < 128) ? (breathPhase * 2) : ((255 - breathPhase) * 2);
      for (uint16_t i = start; i < end; i++) strip.setPixelColor(i, color565ToNeoScaled(color565, breath));
      break;
    }
    case 1: // static
    default:
      for (uint16_t i = start; i < end; i++) strip.setPixelColor(i, color565ToNeo(color565));
      break;
  }
}

static void ringBounds(uint8_t encoderId, uint16_t total, uint16_t &start, uint16_t &end) {
  start = (encoderId == 2) ? RING2_START : RING1_START;
  end = start + RING_SIZE;
  if (start > total) start = total;
  if (end > total) end = total;
}

static uint8_t encoderEffect(uint8_t encoderId) {
  return (encoderId == 2) ? config.store.neopixel_rotate2_effect : config.store.neopixel_rotate1_effect;
}

static uint16_t encoderRotateColor(uint8_t encoderId) {
  return (encoderId == 2) ? config.store.neopixel_rotate2_color : config.store.neopixel_rotate1_color;
}

static bool encoderRotateEnabled(uint8_t encoderId) {
  return (encoderId == 2) ? config.store.neopixel_rotate2_enabled : config.store.neopixel_rotate1_enabled;
}

static void updateOverlayMode(EncoderOverlay &overlay) {
  if (!overlay.active) return;
  displayMode_e currentMode = display.mode();
  if (currentMode == overlay.targetMode) {
    overlay.modeSeen = true;
    return;
  }
  if (overlay.modeSeen) {
    overlay.active = false;
    overlay.modeSeen = false;
  }
}

static void renderEncoderOverlay(uint8_t encoderId, uint16_t total) {
  uint8_t idx = (encoderId == 2) ? 1 : 0;
  EncoderOverlay &overlay = encoderOverlay[idx];
  updateOverlayMode(overlay);
  if (!overlay.active) return;

  uint16_t start, end;
  ringBounds(encoderId, total, start, end);
  uint16_t count = end - start;
  if (count == 0) {
    overlay.active = false;
    return;
  }
  if (overlay.pos < 0) overlay.pos = count - 1;
  if (overlay.pos >= count) overlay.pos = 0;

  uint8_t effect = encoderEffect(encoderId);
  if (effect == 0) return;
  if (effect == 1) {
    strip.setPixelColor(start + overlay.pos, color565ToNeo(encoderRotateColor(encoderId)));
    return;
  }

  uint8_t phase = (encoderId == 2) ? ringPhase2 : ringPhase1;
  phase += overlay.pos * 8;
  renderRing(start, end, effect, encoderRotateColor(encoderId), phase);
}

static void renderRingsAndShow() {
  if (!config.store.neopixel_enabled) {
    strip.clear();
    strip.show();
    return;
  }

  uint16_t total = strip.numPixels();
  uint16_t ring1End = (RING1_START + RING_SIZE < total) ? (RING1_START + RING_SIZE) : total;
  uint16_t ring2End = (RING2_START + RING_SIZE < total) ? (RING2_START + RING_SIZE) : total;

  uint8_t ring2Phase = ringPhase2;
  if (config.store.neopixel_effect == 4 && config.store.neopixel_effect2 == 4) {
    ring2Phase = ringPhase1;
  }

  renderRing(RING1_START, ring1End, config.store.neopixel_effect, config.store.neopixel_enc1_color, ringPhase1);
  renderRing(RING2_START, ring2End, config.store.neopixel_effect2, config.store.neopixel_enc2_color, ring2Phase);

  // Any extra LEDs outside two 8-LED rings are forced off.
  for (uint16_t i = ring2End; i < total; i++) strip.setPixelColor(i, 0);

  renderEncoderOverlay(1, total);
  renderEncoderOverlay(2, total);

  strip.show();
}

void setLedState(LedState state) {
  (void)state;
}

void updateLedState() {
}

void NeoPixel_off() {
  strip.clear();
  strip.show();
}

void ledEncoderRotate(uint8_t encoderId, bool dir) {
  if (!config.store.neopixel_enabled) return;
  encoderId = (encoderId == 2) ? 2 : 1;
  if (!encoderRotateEnabled(encoderId)) return;

  uint16_t start, end;
  ringBounds(encoderId, strip.numPixels(), start, end);
  int count = end - start;
  if (count <= 0) return;

  EncoderOverlay &overlay = encoderOverlay[encoderId - 1];
  overlay.active = true;
  overlay.last = millis();
  overlay.targetMode = (encoderId == 2) ? VOL : STATIONS;
  overlay.modeSeen = (display.mode() == overlay.targetMode);
  overlay.pos += dir ? 1 : -1;
  if (overlay.pos < 0) overlay.pos = count - 1;
  if (overlay.pos >= count) overlay.pos = 0;
  renderRingsAndShow();
}

void ledEncoderRotate(bool dir) {
  ledEncoderRotate(1, dir);
}

void NeoPixel_init() {
  strip.begin();
  strip.setBrightness(config.store.neopixel_brightness);
  strip.clear();
  strip.show();
  renderRingsAndShow();
}

void NeoPixel_setBrightness(uint8_t value) {
  strip.setBrightness(value);
  renderRingsAndShow();
}

void NeoPixel_setEnabled(bool enabled) {
  if (!enabled) {
    NeoPixel_off();
    return;
  }
  NeoPixel_applyConfig();
}

void NeoPixel_setEffect(uint8_t effectId) {
  config.store.neopixel_effect = effectId;
  renderRingsAndShow();
}

void NeoPixel_setEncoderColor(uint8_t encoderId, uint16_t rgb565) {
  if (encoderId == 2) {
    config.store.neopixel_enc2_color = rgb565;
  } else {
    config.store.neopixel_enc1_color = rgb565;
  }
  renderRingsAndShow();
}

void NeoPixel_applyConfig() {
  strip.setBrightness(config.store.neopixel_brightness);
  renderRingsAndShow();
}

void NeoPixel_loop() {
  if (millis() - lastFrame < 40) return;
  lastFrame = millis();
  ringPhase1++;
  ringPhase2 += 2;
  renderRingsAndShow();
}
