#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <Arduino.h>

// ------------------------------------------------------
// PUBLICZNE STANY SEMANTYCZNE LED
// ------------------------------------------------------
// Odzwierciedlają LOGICZNY stan radia,
// a nie konkretne efekty wizualne
enum LedState {
  LED_BOOT,
  LED_PLAYER,
  LED_STATIONS,
  LED_SCREENSAVER,
  LED_LOST,
  LED_NO_WIFI,
  LED_SD_READY
};

// ------------------------------------------------------
// PUBLICZNE API MODUŁU WS2812
// ------------------------------------------------------
#ifdef NEOPIXEL_ON
  void NeoPixel_off();   // gaszenie ledów
#endif
// Inicjalizacja paska WS2812
// Wywołać raz w setup()
void NeoPixel_init();

// Główna pętla animacji LED
// Wywoływać w loop()
void NeoPixel_loop();
void NeoPixel_setBrightness(uint8_t value);
void NeoPixel_setEnabled(bool enabled);
void NeoPixel_setEffect(uint8_t effectId);
void NeoPixel_setEncoderColor(uint8_t encoderId, uint16_t rgb565);
void NeoPixel_applyConfig();

// Automatyczna aktualizacja stanu LED
// Na podstawie display.mode() i network.status
void updateLedState();

// Ręczne wymuszenie stanu LED (opcjonalne)
// Np. przy BUFFERING, PAUSE, MUTE
void setLedState(LedState state);

// Overlay: animacja obrotu enkodera
// encoderId = 1 -> pierwszy pierścień, encoderId = 2 -> drugi pierścień
// dir = true  -> obrót w prawo
// dir = false -> obrót w lewo
void ledEncoderRotate(uint8_t encoderId, bool dir);
void ledEncoderRotate(bool dir);

#endif // NEOPIXEL_H
