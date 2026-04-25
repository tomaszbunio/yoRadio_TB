#pragma once
#ifdef PSFBUFFER

/**
 * @file FlipDigit.h
 * @brief Animacja flip clock – dwie cyfry HH lub MM na jednej karteczce.
 *
 * Zasada działania (klasyczny tablicowy zegar odwracany):
 *   Faza 1 (klatki 0..FLIP_FRAMES/2-1):
 *     Górna połowa STAREJ karteczki opada w dół – efekt kompresji wierszy
 *     (wiersze źródłowe są rozciągane na coraz mniejszy obszar docelowy).
 *     Dolna połowa starej karteczki pozostaje nieruchoma.
 *   Faza 2 (klatki FLIP_FRAMES/2..FLIP_FRAMES-1):
 *     Górna połowa NOWEJ karteczki wznosi się od linii środkowej ku górze –
 *     efekt dekompresji (przyciemnione wiersze nowej karteczki są stopniowo
 *     odkrywane). Dolna połowa nowej karteczki jest widoczna od razu.
 *
 * Każda karteczka trzyma dwa psFrameBuffer:
 *   _bufA – aktualna wartość (wyświetlana statycznie)
 *   _bufB – następna wartość (renderowana przed startem animacji)
 * Podczas animacji klatki są składane piksel po pikselku w buforze tymczasowym
 * i wysyłane bezpośrednio do wyświetlacza przez setAddrWindow/writePixels.
 *
 * Konfiguracja w myoptions.h (wszystkie opcjonalne):
 *   #define FLIP_SPEED_MS   400   // całkowity czas animacji (ms)
 *   #define FLIP_FRAMES      20   // liczba klatek animacji
 *   #define FLIP_CARD_MARGIN  5   // margines góra/dół karteczki (px)
 *   #define FLIP_CORNER_R     4   // promień zaokrąglenia rogów (px)
 *   #define FLIP_SHADOW_OFF   4   // przesunięcie cienia w prawo/dół (px)
 *   #define FLIP_CARD_COLOR   0xFFFF   // kolor tła karteczki
 *   #define FLIP_SHADOW_COLOR 0x4208   // kolor cienia
 *   #define FLIP_LINE_COLOR   0xC618   // kolor linii podziału
 *   #define FLIP_TEXT_COLOR   0x0000   // kolor cyfr
 */

/* ── Wartości domyślne – można nadpisać w myoptions.h ── */
#ifndef FLIP_SPEED_MS
  #define FLIP_SPEED_MS 400       // czas trwania jednej animacji w ms
#endif
#ifndef FLIP_FRAMES
  #define FLIP_FRAMES 20          // liczba klatek na animację
#endif
#ifndef FLIP_CARD_MARGIN
  #define FLIP_CARD_MARGIN FLIP_PANEL_VPAD   // margines góra/dół = VPAD zegara
#endif
#ifndef FLIP_CORNER_R
  #ifdef FLIP_PANEL_RADIUS
    #define FLIP_CORNER_R FLIP_PANEL_RADIUS  // promień rogów = FLIP_PANEL_RADIUS jeśli zdefiniowany
  #else
    #define FLIP_CORNER_R 4
  #endif
#endif
#ifndef FLIP_SHADOW_OFF
  #ifdef FLIP_PANEL_SHADOW_OFF
    #define FLIP_SHADOW_OFF FLIP_PANEL_SHADOW_OFF  // zgodność z poprzednią nazwą makra
  #else
    #define FLIP_SHADOW_OFF 4     // przesunięcie cienia w px
  #endif
#endif
#ifndef FLIP_CARD_COLOR
  #define FLIP_CARD_COLOR   0xFFFF  // białe tło karteczki
#endif
#ifndef FLIP_SHADOW_COLOR
  #define FLIP_SHADOW_COLOR 0x4208  // ciemnoszary cień
#endif
#ifndef FLIP_LINE_COLOR
  #define FLIP_LINE_COLOR   0xC618  // jasnoszara linia środkowa
#endif
#ifndef FLIP_TEXT_COLOR
  #define FLIP_TEXT_COLOR   0x0000  // czarne cyfry
#endif

#include "../tools/psframebuffer.h"
#include "../../core/config.h"
extern const GFXfont *Clock_GFXfontPtr;  // wskaźnik na aktywną czcionkę zegara

class FlipDigit {
public:
  FlipDigit() {}
  ~FlipDigit() { _freeBuffers(); }

  /**
   * @brief Inicjalizuje karteczkę i alokuje dwa framebuffery (PSRAM).
   * @param dspl  wskaźnik na obiekt wyświetlacza (yoDisplay)
   * @param x     pozycja X lewego boku karteczki na ekranie
   * @param y     baseline czcionki – dół tekstu (jak setCursor y w GFX)
   * @param w     szerokość bufora = szerokość karteczki + FLIP_SHADOW_OFF
   * @param h     wysokość tekstu (_timeheight z ClockWidget)
   *
   * Bufor ma wysokość h + 2*FLIP_CARD_MARGIN (margines góra i dół).
   * Pozycja górna bufora na ekranie: y - h - FLIP_CARD_MARGIN.
   */
  void init(yoDisplay *dspl, int16_t x, int16_t y, int16_t w, int16_t h) {
    _dspl = dspl;
    _x = x; _y = y; _w = w; _h = h;
    strncpy(_current, "  ", 3);
    strncpy(_next,    "  ", 3);
    _flipping  = false;
    _frame     = 0;
    _lastFrame = 0;
    _freeBuffers();
    // Bufor obejmuje karteczkę + margines góra/dół
    int16_t bufH = h + 2 * FLIP_CARD_MARGIN;
    _bufA = new psFrameBuffer(w, bufH);
    _bufB = new psFrameBuffer(w, bufH);
    // Oba buffery mapowane na ten sam obszar ekranu (różnią się zawartością)
    _bufA->begin(dspl, x, y - h - FLIP_CARD_MARGIN, w, bufH, 0x0000);
    _bufB->begin(dspl, x, y - h - FLIP_CARD_MARGIN, w, bufH, 0x0000);
  }

  /**
   * @brief Ustawia wartość natychmiast (bez animacji) i odświeża ekran.
   */
  void setValue(const char *val) {
    if (!_bufA || !_bufA->ready()) return;
    strncpy(_current, val, 2); _current[2] = '\0';
    _flipping = false; _frame = 0;
    _renderStr(_bufA, _current);  // renderuj na bufor A
    _bufA->display();             // wyślij na ekran
  }

  /**
   * @brief Uruchamia animację przejścia do nowej wartości.
   *        Jeśli animacja już trwa lub wartość niezmieniona – ignoruje.
   */
  void flipTo(const char *val) {
    if (_flipping) return;                          // animacja w toku – ignoruj
    if (strncmp(val, _current, 2) == 0) return;    // ta sama wartość – nic do roboty
    if (!_bufA || !_bufB || !_bufA->ready() || !_bufB->ready()) {
      setValue(val); return;   // brak PSRAM – wyświetl bez animacji
    }
    strncpy(_next, val, 2); _next[2] = '\0';
    _renderStr(_bufB, _next);  // przygotuj nową wartość w buforze B
    _flipping  = true;
    _frame     = 0;
    _lastFrame = millis();
  }

  /**
   * @brief Pętla animacji – wywoływać jak najczęściej (np. co ~10 ms z tick()).
   *        Steruje tempem klatek na podstawie FLIP_SPEED_MS / FLIP_FRAMES.
   */
  void loop() {
    if (!_flipping) return;
    uint32_t now = millis();
    // Czas między klatkami = całkowity czas animacji / liczba klatek
    if (now - _lastFrame < (uint32_t)(FLIP_SPEED_MS / FLIP_FRAMES)) return;
    _lastFrame = now;
    _drawFrame(_frame);
    _frame++;
    if (_frame >= FLIP_FRAMES) {
      // Animacja zakończona – bufor A = nowa wartość, wyświetl statycznie
      _flipping = false;
      strncpy(_current, _next, 2); _current[2] = '\0';
      _renderStr(_bufA, _current);
      _bufA->display();
    }
  }

  bool        isFlipping() const { return _flipping; }
  const char* current()    const { return _current; }
  void        freeBuffers()      { _freeBuffers(); }
  void        setLabel(const char *lbl) {
    if (_bufA) _bufA->setLabel(lbl);
    if (_bufB) _bufB->setLabel(lbl);
  }

private:
  yoDisplay     *_dspl    = nullptr;
  int16_t        _x, _y, _w, _h;         // pozycja i wymiary karteczki
  char           _current[3] = "  ";     // aktualnie wyświetlana wartość
  char           _next[3]    = "  ";     // wartość docelowa animacji
  bool           _flipping   = false;    // czy animacja trwa
  uint8_t        _frame      = 0;        // numer bieżącej klatki
  uint32_t       _lastFrame  = 0;        // czas ostatniej klatki (ms)
  psFrameBuffer *_bufA = nullptr;        // bufor aktualnej wartości
  psFrameBuffer *_bufB = nullptr;        // bufor następnej wartości

  void _freeBuffers() {
    if (_bufA) { delete _bufA; _bufA = nullptr; }
    if (_bufB) { delete _bufB; _bufB = nullptr; }
  }

  // ── Renderowanie karteczki do bufora ──────────────────────────────────────

  /**
   * @brief Rysuje karteczkę z cyframi do wskazanego psFrameBuffer.
   *
   * Układ bufora (wysokość H = _h + 2*MARGIN):
   *   [0 .. MARGIN-1]        – górny margines (czarne tło)
   *   [MARGIN .. H-MARGIN-1] – karteczka: cień + biały prostokąt + cyfry
   *   [H-MARGIN .. H-1]      – dolny margines (cień)
   *
   * Linia środkowa dzieli karteczkę na górną i dolną połowę.
   */
  void _renderStr(psFrameBuffer *buf, const char *str) {
    if (!buf || !buf->ready()) return;
    const int16_t W    = _w;
    const int16_t H    = _h + 2 * FLIP_CARD_MARGIN;
    const int16_t sOff = FLIP_SHADOW_OFF;  // przesunięcie cienia
    const int16_t r    = FLIP_CORNER_R;    // promień rogów

    // Tło całego bufora – czarne (= kolor tła ekranu)
    buf->fillRect(0, 0, W, H, 0x0000);
    // Cień: przesunięty prostokąt z zaokrąglonymi rogami
    buf->fillRoundRect(sOff, sOff, W - sOff, H - sOff, r, FLIP_SHADOW_COLOR);
    // Karteczka: biały prostokąt na wierzchu cienia
    buf->fillRoundRect(0, 0, W - sOff, H - sOff, r, config.theme.flipCard);
    // Linia środkowa – poziomy podział na górną i dolną połowę (2 px grubości)
    int16_t lineY = (H - sOff) / 2;
    buf->drawFastHLine(0, lineY,     W - sOff, FLIP_LINE_COLOR);
    buf->drawFastHLine(0, lineY + 1, W - sOff, FLIP_LINE_COLOR);

    // Ustawienie czcionki i koloru tekstu
    buf->setFont(Clock_GFXfontPtr);
    buf->setTextSize(1);
    buf->setTextColor(config.theme.flipText);

    // Pomiar szerokości tekstu przez getTextBounds (zwraca rzeczywiste piksele)
    int16_t bx, by;
    uint16_t bw, bh;
    buf->getTextBounds(str, 0, H, &bx, &by, &bw, &bh);

    // Wyśrodkowanie poziome: cx przesuwa kursor tak, by środek wizualny tekstu
    // pokrył się ze środkiem karteczki (W - sOff)
    int16_t cx = ((W - sOff) - (int16_t)bw) / 2 - bx;
    // Baseline: FLIP_CARD_MARGIN px od dolnej krawędzi karteczki
    int16_t baseline = H - sOff - FLIP_CARD_MARGIN;

    buf->setCursor(cx, baseline);
    buf->print(str);
  }

  // ── Animacja klatka po klatce ──────────────────────────────────────────────

  /**
   * @brief Renderuje jedną klatkę animacji i wysyła ją bezpośrednio na ekran.
   *
   * Klatki podzielone na dwie fazy (każda po FLIP_FRAMES/2 klatek):
   *
   * Faza 1 (frame < half): górna połowa STAREJ karteczki opada.
   *   - flapH maleje od midY do 0 (klapka kurczy się)
   *   - wiersze [0, midY-flapH):  kopiowane bez zmian ze starego bufora
   *   - wiersze [midY-flapH, midY): stara karteczka skompresowana do flapH wierszy
   *     (każdy docelowy wiersz pobiera proporcjonalny wiersz ze źródła)
   *   - wiersze [midY, H):        dolna połowa starej karteczki bez zmian
   *
   * Faza 2 (frame >= half): górna połowa NOWEJ karteczki wznosi się.
   *   - flapH rośnie od 0 do midY (klapka odkrywa się)
   *   - wiersze [0, midY):        nowa karteczka (górna połowa) – widoczna
   *   - wiersze [midY, midY+flapH): nowa karteczka dekompresowana + przyciemniona
   *     (symuluje cień opadającej klapki)
   *   - wiersze [midY+flapH, H):  dolna połowa nowej karteczki bez zmian
   *
   * Wynik składany w tymczasowym buforze RAM, wysyłany jednym wywołaniem
   * setAddrWindow/writePixels (minimalna liczba transakcji SPI).
   */
  void _drawFrame(uint8_t frame) {
    if (!_bufA || !_bufB || !_bufA->ready() || !_bufB->ready()) return;

    const int16_t H    = _h + 2 * FLIP_CARD_MARGIN;  // całkowita wysokość bufora
    const int16_t midY = H / 2;                        // linia środkowa
    const uint8_t half = FLIP_FRAMES / 2;              // granica między fazami

    // Tymczasowy bufor klatki – alokowany w RAM zwykłym (nie PSRAM)
    uint16_t *fb = (uint16_t*)malloc(_w * H * sizeof(uint16_t));
    if (!fb) return;

    if (frame < half) {
      // ── Faza 1: stara karteczka opada ──
      // flapH = wysokość widocznej klapki (maleje od midY do 0)
      int16_t flapH = midY - (int16_t)((int32_t)midY * frame / half);
      for (int16_t row = 0; row < H; row++) {
        if (row < midY - flapH) {
          // Obszar powyżej klapki – pusty (tło ekranu z bufora A)
          _copyRow(_bufA, row, fb, row);
        } else if (row < midY) {
          // Klapka: kompresja wierszy starej karteczki do flapH wierszy
          // srcRow mapuje liniowo [0, flapH) → [0, midY-1)
          int16_t srcRow = (flapH > 0)
            ? (int16_t)((int32_t)(row - (midY - flapH)) * (midY - 1) / flapH)
            : 0;
          // Przyciemnione – symuluje cień opadającej klapki
          _copyRowDimmed(_bufA, constrain(srcRow, 0, midY - 1), fb, row);
        } else {
          // Dolna połowa – stara karteczka bez zmian
          _copyRow(_bufA, row, fb, row);
        }
      }
    } else {
      // ── Faza 2: nowa karteczka wznosi się ──
      // flapH = wysokość już odkrytej nowej klapki (rośnie od 0 do midY)
      int16_t flapH = (int16_t)((int32_t)midY * (frame - half) / half);
      for (int16_t row = 0; row < H; row++) {
        if (row < midY) {
          // Górna połowa nowej karteczki – widoczna od razu
          _copyRow(_bufB, row, fb, row);
        } else if (row < midY + flapH) {
          // Klapka nowej karteczki: dekompresja + przyciemnienie
          // srcRow mapuje [midY, midY+flapH) → [midY-1, 0) (odwrotnie)
          int16_t srcRow = (flapH > 0)
            ? midY - 1 - (int16_t)((int32_t)(row - midY) * (midY - 1) / flapH)
            : midY - 1;
          _copyRowDimmed(_bufB, constrain(srcRow, 0, midY - 1), fb, row);
        } else {
          // Dolna połowa nowej karteczki – widoczna bez animacji
          _copyRow(_bufB, row, fb, row);
        }
      }
    }

    // Wyślij tylko zmienioną połowę bufora:
    //   Faza 1: dolna połowa niezmieniona na ekranie (z ostatniego setValue) → wysyłaj tylko górną [0, midY)
    //   Faza 2 frame==half: ustal nową górną połowę jednym pełnym transferem
    //   Faza 2 frame>half:  górna połowa już ustalona → wysyłaj tylko dolną [midY, H)
    int16_t screenY = _y - _h - FLIP_CARD_MARGIN;
#ifdef DEBUG_SPI_TIMING
    int64_t t0 = esp_timer_get_time();
#endif
    _dspl->startWrite();
    if (frame < half) {
      _dspl->setAddrWindow(_x, screenY, _w, midY);
      _dspl->writePixels(fb, _w * midY);
    } else if (frame == half) {
      _dspl->setAddrWindow(_x, screenY, _w, H);
      _dspl->writePixels(fb, _w * H);
    } else {
      _dspl->setAddrWindow(_x, screenY + midY, _w, H - midY);
      _dspl->writePixels(fb + _w * midY, _w * (H - midY));
    }
    _dspl->endWrite();
#ifdef DEBUG_SPI_TIMING
    int16_t sentH = (frame == half) ? H : midY;
    int16_t sentY = (frame > half)  ? screenY + midY : screenY;
    Serial.printf("[SPI] FlipDigit %dx%d @ (%d,%d): %lld us\n", _w, sentH, _x, sentY, esp_timer_get_time() - t0);
#endif
    free(fb);
  }

  /**
   * @brief Kopiuje jeden wiersz pikseli z psFrameBuffer do bufora docelowego.
   */
  void _copyRow(psFrameBuffer *src, int16_t srcRow,
                uint16_t *dst, int16_t dstRow) {
    const uint16_t *row = src->getRow(srcRow);
    uint16_t *d = dst + dstRow * _w;
    if (!row) { memset(d, 0, _w * sizeof(uint16_t)); return; }
    memcpy(d, row, _w * sizeof(uint16_t));
  }

  /**
   * @brief Kopiuje wiersz pikseli z przyciemnieniem (każdy kanał RGB >> 1).
   *        Używane do symulacji cienia opadającej/wznoszącej się klapki.
   */
  void _copyRowDimmed(psFrameBuffer *src, int16_t srcRow,
                      uint16_t *dst, int16_t dstRow) {
    const uint16_t *row = src->getRow(srcRow);
    uint16_t *d = dst + dstRow * _w;
    if (!row) { memset(d, 0, _w * sizeof(uint16_t)); return; }
    for (int16_t i = 0; i < _w; i++) {
      uint16_t px = row[i];
      // RGB565: wyodrębnij kanały, podziel przez 2 (przyciemnienie 50%)
      uint8_t r5 = ((px >> 11) & 0x1F) >> 1;
      uint8_t g6 = ((px >>  5) & 0x3F) >> 1;
      uint8_t b5 = ( px        & 0x1F) >> 1;
      d[i] = (r5 << 11) | (g6 << 5) | b5;
    }
  }
};

#else
// Stub gdy PSFBUFFER nie jest włączony – klasa bezoperacyjna
class FlipDigit {
public:
  void init(void*, int16_t, int16_t, int16_t, int16_t) {}
  void setValue(const char*) {}
  void flipTo(const char*) {}
  void loop() {}
  bool isFlipping() const { return false; }
  const char* current() const { return "  "; }
  void freeBuffers() {}
};
#endif // PSFBUFFER
