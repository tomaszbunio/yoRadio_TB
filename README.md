# yoRadio – Tomasz Bunio fork

Moje modyfikacje projektu yoRadio autorstwa [e2002](https://github.com/e2002/yoradio)
i [VaraiTamas](https://github.com/VaraiTamas/yoRadio).

## Moje zmiany: v0.8.7_TB

- Flip clock
- logo stacji radiowych zamiast pogody i bitrate
- automatyczna zmiana czasu z wykorzystaniem TIMEZONE_POSIX

### Konwerter logo stacji (`convert_logos.bat`)

- konwertuje wszystkie pliki `.png` z katalogu `data/` do binarnego formatu RGB565 (little-endian, 120×90 px)
- pliki `.raw` zapisywane są bezpośrednio do `data/www/`
- aby logo się wczytało, uploadujemy pliki `.raw` przez webboard (Settings → BOARD); nazwa pliku musi być identyczna z nazwą stacji wyświetlaną na górze ekranu
- wymaga biblioteki Pillow: `pip install Pillow`

### Konwerter boot logo (`convert_bootlogo.bat`)

- konwertuje plik `logos_src/logo.png` do nagłówków C z tablicą RGB565, wkompilowanych w firmware
- generuje dwa pliki:
  - `bootlogo80x80.h` – dla wyświetlaczy 480×320 (ILI9488, ST7796)
  - `bootlogo54x54.h` – dla wyświetlaczy 320×240 (ILI9341, ST7789)
- po konwersji należy skompilować i wgrać firmware
- wymaga biblioteki Pillow: `pip install Pillow`

---

## Moje zmiany: v0.8.6_TB

- Podświetlenie stacji na playliście
- Obsługa diod adresowanych WS2812B
- Timer ON/OFF sterowany przez www
- Wybór czcionki zegara przez www
- Zmiana kolorów (themes) przez www
- Odczyt nazwy stacji z playlisty
- Sleep z dwukliku enkodera

## Oryginalna dokumentacja:

https://github.com/VaraiTamas/yoRadio
