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
#include "driver/rtc_io.h"
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

  // 5. Ustabilizuj pin wybudzania (enkoder) przed deep sleep
  pinMode(WAKEUP_PIN, INPUT_PULLUP);
  rtc_gpio_pullup_en((gpio_num_t)WAKEUP_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)WAKEUP_PIN);

  // 6. Wybudzanie z deep sleep realizuje tylko przycisk enkodera.
  // 7. Krótkie opóźnienie dla stabilności
  delay(100);

  // 8. Deep sleep jest wybudzany tym samym przyciskiem (LOW).
  // Jeśli przycisk jest jeszcze wciśnięty po dwukliku, ESP może obudzić się natychmiast.
  // Poczekaj krótko na zwolnienie przycisku; jeśli nadal LOW, przerwij usypianie.
  if (digitalRead(WAKEUP_PIN) == LOW) {
    uint32_t t0 = millis();
    while (digitalRead(WAKEUP_PIN) == LOW && (millis() - t0) < 1200) {
      delay(10);
    }
    if (digitalRead(WAKEUP_PIN) == LOW) {
      Serial.println("   - Wake pin nadal LOW, anuluję usypianie");
      display.putRequest(NEWMODE, PLAYER);
      return;
    }
  }

  // 9. Konfiguruj wybudzanie przez pin enkodera.
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, WAKEUP_LEVEL);
  Serial.println("   - Wakeup pin: ENC_BTNB (LOW)");

  // 10. Zasypiam
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
