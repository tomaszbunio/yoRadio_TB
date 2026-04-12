# yoRadio – Tomasz Bunio fork

Moje modyfikacje projektu yoRadio autorstwa [e2002](https://github.com/e2002/yoradio)
i [VaraiTamas](https://github.com/VaraiTamas/yoRadio).

## Moje zmiany: v0.8.7_TB
- Flip clock
- nowa biblioteka audio 3.4.5l
- logo stacji radiowych zamiast pogody i bitrate
- automatyczna zmiana czasu z wykorzystaniem TIMEZONE_POSIX

Opis do konwertera logo.
- skrypt jest konwerterem logo stacji radiowych z formatu PNG do binarnego formatu RGB565, gotowego do wyświetlenia na wyświetlaczu TFT (sterownik ILI9486_SPI).
- konwertuje wszystkie pliki .png z katalogu yoRadio/data/ do plików .raw (binarny RGB565, little-endian).
- wymaga biblioteki Pillow (pip install Pillow).
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
