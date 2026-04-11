#pragma once
#ifdef STATION_LOGO_WIDGET

/**
 * @file StationLogoWidget.h
 * @brief Widget wyświetlający logo stacji radiowej z pliku binarnego RGB565 na SPIFFS.
 *
 * Format pliku: raw RGB565, little-endian, WIDTH*HEIGHT*2 bajtów.
 * Konwersja PNG -> .raw: skrypt convert_logos.py w katalogu projektu.
 *
 * Pliki .raw umieszczane są na SPIFFS w /www/ (webboard wgrywa je tam automatycznie).
 * Nazwa pliku = znormalizowana nazwa stacji + ".raw":
 *   "Eska Rock" → /www/Eska_Rock.raw
 *
 * Plik domyślny (gdy brak logo stacji): /www/logo_default.raw
 * Jeśli i ten nie istnieje – obszar widgetu czyszczony jest tłem.
 *
 * Konfiguracja w myoptions.h:
 *   #define STATION_LOGO_WIDGET
 *   #define STATION_LOGO_X   10
 *   #define STATION_LOGO_Y  100
 *   #define STATION_LOGO_W  160
 *   #define STATION_LOGO_H  120
 */

#include <SPIFFS.h>
#include "../../core/options.h"
#include "../dspcore.h"

/* ── Wartości domyślne pozycji – można nadpisać w myoptions.h ── */
#ifndef STATION_LOGO_X
  #define STATION_LOGO_X 0
#endif
#ifndef STATION_LOGO_Y
  #define STATION_LOGO_Y 100
#endif
#ifndef STATION_LOGO_W
  #define STATION_LOGO_W 160
#endif
#ifndef STATION_LOGO_H
  #define STATION_LOGO_H 120
#endif

extern DspCore dsp;

class StationLogoWidget {
public:
    StationLogoWidget() {}

    /**
     * @brief Inicjalizuje SPIFFS (jeśli jeszcze nie jest zamontowany).
     *        Wywołać raz podczas Display::init().
     */
    void init() {
        if (!SPIFFS.begin(false)) {
            Serial.println("[StationLogo] SPIFFS mount failed");
            return;
        }
        // Listing plików .raw w /www/
        Serial.println("[StationLogo] Pliki .raw na SPIFFS (/www/):");
        File root = SPIFFS.open("/www");
        if (root && root.isDirectory()) {
            File f = root.openNextFile();
            int count = 0;
            while (f) {
                String name = f.name();
                if (name.endsWith(".raw")) {
                    Serial.printf("  %s  (%u B)\n", f.name(), (unsigned)f.size());
                    count++;
                }
                f = root.openNextFile();
            }
            Serial.printf("[StationLogo] Razem: %d plikow .raw\n", count);
        } else {
            Serial.println("[StationLogo] Brak katalogu /www lub blad otwarcia");
        }
    }

    /**
     * @brief Wyświetla logo dla podanej nazwy stacji.
     * @param stationName  Nazwa stacji z config.station.name
     */
    void setStation(const char *stationName) {
        char path[72];
        _buildPath(stationName, path, sizeof(path));

        Serial.printf("[StationLogo] stacja='%s' -> plik='%s' istnieje=%d\n",
                      stationName, path, SPIFFS.exists(path) ? 1 : 0);

        if (_drawBin(path)) return;

        // Fallback: szukaj logo nadawcy nadrzędnego
#ifdef STATION_LOGO_FALLBACKS
        {
            struct _FB { const char *prefix; const char *file; };
            static const _FB fallbacks[] = { STATION_LOGO_FALLBACKS };
            // znormalizowana nazwa stacji (lowercase) do porównania prefiksu
            char norm[48] = {};
            _buildStem(stationName, norm, sizeof(norm));
            for (size_t i = 0; i < sizeof(fallbacks)/sizeof(fallbacks[0]); i++) {
                size_t plen = strlen(fallbacks[i].prefix);
                if (strncmp(norm, fallbacks[i].prefix, plen) == 0) {
                    char fbpath[40];
                    snprintf(fbpath, sizeof(fbpath), "/www/%s.raw", fallbacks[i].file);
                    Serial.printf("[StationLogo] fallback -> '%s'\n", fbpath);
                    if (_drawBin(fbpath)) return;
                }
            }
        }
#endif

        // Ostatni resort: logo domyślne
        if (!_drawBin("/www/logo_default.raw")) {
            clear();
        }
    }

    /**
     * @brief Czyści obszar widgetu kolorem tła.
     */
    void clear() {
        dsp.fillRect(STATION_LOGO_X, STATION_LOGO_Y,
                     STATION_LOGO_W, STATION_LOGO_H, _bgColor);
    }

    /**
     * @brief Ustawia kolor tła (wywoływać po zmianie motywu).
     */
    void setBgColor(uint16_t color) { _bgColor = color; }

private:
    uint16_t _bgColor = 0x0000;

    /**
     * @brief Normalizuje nazwę stacji do ścieżki pliku .raw w /www/.
     *        "Eska Rock" → "/www/Eska_Rock.raw"
     */
    // Zwraca ASCII odpowiednik polskiego znaku UTF-8 (2 bajty b1,b2), lub 0 jeśli nieznany
    char _translitPL(uint8_t b1, uint8_t b2) {
        if (b1 == 0xC3) {
            if (b2 == 0x93) return 'O'; // Ó
            if (b2 == 0xB3) return 'o'; // ó
        }
        if (b1 == 0xC4) {
            if (b2 == 0x84) return 'A'; // Ą
            if (b2 == 0x85) return 'a'; // ą
            if (b2 == 0x86) return 'C'; // Ć
            if (b2 == 0x87) return 'c'; // ć
            if (b2 == 0x98) return 'E'; // Ę
            if (b2 == 0x99) return 'e'; // ę
        }
        if (b1 == 0xC5) {
            if (b2 == 0x81) return 'L'; // Ł
            if (b2 == 0x82) return 'l'; // ł
            if (b2 == 0x83) return 'N'; // Ń
            if (b2 == 0x84) return 'n'; // ń
            if (b2 == 0x9A) return 'S'; // Ś
            if (b2 == 0x9B) return 's'; // ś
            if (b2 == 0xB9) return 'Z'; // Ź
            if (b2 == 0xBA) return 'z'; // ź
            if (b2 == 0xBB) return 'Z'; // Ż
            if (b2 == 0xBC) return 'z'; // ż
        }
        return 0;
    }

    // Zwraca samą znormalizowaną nazwę (bez /www/ i .raw) – do porównania prefiksów
    void _buildStem(const char *name, char *out, size_t maxLen) {
        size_t oi = 0;
        size_t stem_len = 0;
        static const size_t STEM_MAX = 22;
        for (size_t i = 0; name[i] && oi < maxLen - 1 && stem_len < STEM_MAX; ) {
            uint8_t c = (uint8_t)name[i];
            if (c >= 0xC2 && name[i + 1]) {
                char asc = _translitPL(c, (uint8_t)name[i + 1]);
                if (asc) {
                    out[oi++] = (asc >= 'A' && asc <= 'Z') ? asc + 32 : asc;
                    stem_len++; i += 2; continue;
                }
            }
            if (c == ' ' || c == '-') {
                if (oi == 0 || out[oi - 1] != '_') { out[oi++] = '_'; stem_len++; }
            } else if (c >= 'A' && c <= 'Z') { out[oi++] = c + 32; stem_len++; }
            else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                out[oi++] = c; stem_len++;
            }
            i++;
        }
        out[oi] = '\0';
    }

    void _buildPath(const char *name, char *out, size_t maxLen) {
        // Limit SPIFFS: 31 znaków. /www/ (5) + nazwa (max 22) + .raw (4) = 31
        static const size_t STEM_MAX = 22;
        size_t oi = 0;
        const char *prefix = "/www/";
        while (*prefix && oi < maxLen - 5) out[oi++] = *prefix++;

        size_t stem_len = 0;
        for (size_t i = 0; name[i] && oi < maxLen - 5 && stem_len < STEM_MAX; ) {
            uint8_t c = (uint8_t)name[i];
            // Obsługa 2-bajtowych sekwencji UTF-8 (polskie znaki)
            if (c >= 0xC2 && name[i + 1]) {
                char asc = _translitPL(c, (uint8_t)name[i + 1]);
                if (asc) { out[oi++] = (asc >= 'A' && asc <= 'Z') ? asc + 32 : asc; stem_len++; i += 2; continue; }
            }
            if (c == ' ' || c == '-') {
                // spacja i myślnik → '_', ale nie podwajaj
                if (oi == 0 || out[oi - 1] != '_') { out[oi++] = '_'; stem_len++; }
            }
            else if (c >= 'A' && c <= 'Z')         { out[oi++] = c + 32; stem_len++; } // lowercase
            else if ((c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     c == '_')                     { out[oi++] = c; stem_len++; }
            i++;
        }
        strlcpy(out + oi, ".raw", maxLen - oi);
    }

    /**
     * @brief Wczytuje plik raw RGB565 i wyświetla go na ekranie wiersz po wierszu.
     *        Format: WIDTH*HEIGHT pikseli po 2 bajty (little-endian uint16_t).
     *        Driver ILI9486_SPI::writePixels() sam zamienia bajty przed SPI.
     * @return true – sukces, false – plik nie istnieje lub błąd odczytu
     */
    bool _drawBin(const char *path) {
        if (!SPIFFS.exists(path)) return false;

        File f = SPIFFS.open(path, "r");
        if (!f) return false;

        // Oblicz ile pełnych wierszy można odczytać z pliku
        const size_t rowBytes = STATION_LOGO_W * 2;
        int16_t rows = (int16_t)(f.size() / rowBytes);
        if (rows <= 0) {
            Serial.printf("[StationLogo] Plik %s pusty lub za mały (%u B)\n",
                          path, (unsigned)f.size());
            f.close();
            return false;
        }
        if (rows > STATION_LOGO_H) rows = STATION_LOGO_H;
        // Bufor jednego wiersza
        static uint16_t rowBuf[STATION_LOGO_W];

        // Wyczyść obszar przed rysowaniem
        dsp.fillRect(STATION_LOGO_X, STATION_LOGO_Y,
                     STATION_LOGO_W, STATION_LOGO_H, _bgColor);

        dsp.startWrite();
        for (int16_t row = 0; row < rows; row++) {
            if (f.read((uint8_t*)rowBuf, rowBytes) != rowBytes) {
                Serial.printf("[StationLogo] Błąd odczytu wiersza %d\n", row);
                break;
            }
            dsp.setAddrWindow(STATION_LOGO_X, STATION_LOGO_Y + row,
                              STATION_LOGO_W, 1);
            dsp.writePixels(rowBuf, STATION_LOGO_W);
        }
        dsp.endWrite();

        f.close();
        return true;
    }
};

#endif // STATION_LOGO_WIDGET
