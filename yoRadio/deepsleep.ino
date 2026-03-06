// ===========================================
// RĘCZNE UŚPIENIE RADIA - TYLKO DWUKLIK!
// ===========================================

#include "src/core/options.h"
#include "src/core/display.h"
#include "src/core/network.h"
#include "src/Scheduler/scheduler.h"

#define WAKEUP_PIN ENC_BTNB     // Przycisk enkodera
#define WAKEUP_LEVEL LOW        // Budzi na stan niski

// ---------------------------------------------------------------------
// GŁÓWNA FUNKCJA UŚPIENIA - wywoływana z onBtnDoubleClick i schedulera
// ---------------------------------------------------------------------
void goToSleep() {
	
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

  Serial.println("\n💤💤💤 RĘCZNE UŚPIENIE 💤💤💤");
  Serial.printf("Czas: %lu ms\n", millis());
  
  // 1. Wyłącz podświetlenie LCD
  if (BRIGHTNESS_PIN != 255) {
    analogWrite(BRIGHTNESS_PIN, 0);
    Serial.println("   - Podświetlenie OFF");
  }
  
  // 2. Uśpij wyświetlacz (jeśli wspiera)
  if (display.deepsleep()) {
    Serial.println("   - Wyświetlacz uśpiony");
  }
  
  // 3. Konfiguruj wybudzanie - kliknięcie enkodera
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_PIN, WAKEUP_LEVEL);
  Serial.println("   - Wakeup pin: ENC_BTNB (LOW)");
  
  // 4. Krótkie opóźnienie dla stabilności
  delay(100);
  
  // 5. IDZIEMY SPAĆ!
  Serial.println("💤 Zasypiam... Do zobaczenia!");
  Serial.flush();  // Wyślij wszystkie logi przed snem
  
  esp_deep_sleep_start();
}

// ---------------------------------------------------------------------
// PUSTE CALLBACKI - automatyczny sleep WYŁĄCZONY!
// ---------------------------------------------------------------------
void player_on_start_play() { 
  // Celowo puste - nie chcemy automatu
}

void player_on_stop_play() { 
  // Celowo puste - nie chcemy automatu
}

// ===========================================
// KONIEC - TYLKO RĘCZNE UŚPIENIE
// ===========================================
