/**
 * @file deepsleep.cpp
 * @brief Implementacja ręcznego uśpienia ESP32 (deep sleep).
 *
 * Uśpienie wywoływane przez dwuklik enkodera lub scheduler.
 * Automatyczne uśpienie jest wyłączone (puste callbacki).
 */

#include "deepsleep.h"
#include "options.h"
#include "display.h"
#include "network.h"
#include "../Scheduler/scheduler.h"
#ifdef NEOPIXEL_ON
  #include "../NeoPixel/NeoPixel.h"
#endif

// ---------------------------------------------------------------------
// GŁÓWNA FUNKCJA UŚPIENIA
// ---------------------------------------------------------------------

void goToSleep() {
  Serial.println("\n💤💤💤 RĘCZNE UŚPIENIE 💤💤💤");
  Serial.printf("Czas: %lu ms\n", millis());

  // 1. Ustaw wybudzenie przez scheduler jeśli aktywny
  if (scheduler.enabled && scheduler.eventCount > 0) {
    uint32_t secondsToNext = scheduler.secondsToNext(
      (network.timeinfo.tm_wday + 6) % 7,
      network.timeinfo.tm_hour,
      network.timeinfo.tm_min
    );
    if (secondsToNext > 0) {
      esp_sleep_enable_timer_wakeup((uint64_t)secondsToNext * 1000000ULL);
      Serial.printf("   - Scheduler wakeup za %d sekund\n", secondsToNext);
    }
  }

  // 2. Wyłącz podświetlenie LCD
  if (BRIGHTNESS_PIN != 255) {
    analogWrite(BRIGHTNESS_PIN, 0);
    Serial.println("   - Podświetlenie OFF");
  }

  // 3. Wyłącz LEDy NeoPixel
  #ifdef NEOPIXEL_ON
    NeoPixel_off();
    Serial.println("   - NeoPixel OFF");
  #endif

  // 4. Uśpij wyświetlacz
  if (display.deepsleep()) {
    Serial.println("   - Wyświetlacz uśpiony");
  }

  // 5. Konfiguruj wybudzanie przez pin enkodera
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, WAKEUP_LEVEL);
  Serial.println("   - Wakeup pin: ENC_BTNB (LOW)");

  // 6. Krótkie opóźnienie dla stabilności
  delay(100);

  // 7. Zasypiam
  Serial.println("💤 Zasypiam... Do zobaczenia!");
  Serial.flush();
  esp_deep_sleep_start();
}

// ---------------------------------------------------------------------
// PUSTE CALLBACKI – automatyczne uśpienie wyłączone
// ---------------------------------------------------------------------

void player_on_start_play() {
  // Celowo puste – nie chcemy automatycznego uśpienia
}

void player_on_stop_play() {
  // Celowo puste – nie chcemy automatycznego uśpienia
}
