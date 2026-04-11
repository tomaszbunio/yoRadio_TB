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

// Automatyczna aktualizacja stanu LED
// Na podstawie display.mode() i network.status
void updateLedState();

// Ręczne wymuszenie stanu LED (opcjonalne)
// Np. przy BUFFERING, PAUSE, MUTE
void setLedState(LedState state);

// Overlay: animacja obrotu enkodera
// dir = true  -> obrót w prawo
// dir = false -> obrót w lewo
void ledEncoderRotate(bool dir);

#endif // NEOPIXEL_H