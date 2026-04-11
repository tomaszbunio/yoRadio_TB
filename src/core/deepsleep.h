#pragma once

/**
 * @file deepsleep.h
 * @brief Obsługa ręcznego uśpienia ESP32 (deep sleep).
 *
 * Uśpienie jest wyłącznie ręczne – przez dwuklik enkodera lub scheduler.
 * Wybudzenie następuje przez pin ENC_BTNB (stan niski).
 */

// Pin i poziom wybudzenia
#define WAKEUP_PIN   ENC_BTNB  ///< Pin wybudzający – przycisk enkodera
#define WAKEUP_LEVEL LOW       ///< Stan wybudzający – niski

/**
 * @brief Usypia ESP32 w trybie deep sleep.
 *
 * Kolejność działań:
 *  1. Oblicza czas do następnego zdarzenia schedulera (jeśli aktywny)
 *  2. Wyłącza podświetlenie LCD i LEDy NeoPixel
 *  3. Usypia wyświetlacz
 *  4. Konfiguruje wybudzanie przez pin ENC_BTNB
 *  5. Wywołuje esp_deep_sleep_start()
 */
void goToSleep();

/**
 * @brief Callback wywoływany przy starcie odtwarzania.
 * Celowo pusty – automatyczne uśpienie wyłączone.
 */
void player_on_start_play();

/**
 * @brief Callback wywoływany przy zatrzymaniu odtwarzania.
 * Celowo pusty – automatyczne uśpienie wyłączone.
 */
void player_on_stop_play();
