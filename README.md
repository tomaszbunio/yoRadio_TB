# yoRadio – Tomasz Bunio fork

Moje modyfikacje projektu yoRadio autorstwa [e2002](https://github.com/e2002/yoradio)
i [VaraiTamas](https://github.com/VaraiTamas/yoRadio).

## Moje zmiany: v0.8.7_TB - testowałem tylko na lcd ILI9488.

- Flip clock
- logo stacji radiowych zamiast pogody i bitrate
- automatyczna zmiana czasu z wykorzystaniem TIMEZONE_POSIX
- rozdzielenie tasków dsp i audio, aby nie rywalizowały o zasoby rdzenia
- upłynnienie scrollingu title (co 3px zamiast co 7px).

Opis do konwertera logo.

- skrypt jest konwerterem logo stacji radiowych z formatu PNG do binarnego formatu RGB565, gotowego do wyświetlenia na wyświetlaczu TFT (sterownik ILI9486_SPI).
- konwertuje wszystkie pliki .png z katalogu yoRadio/data/ do plików .raw (binarny RGB565, little-endian) do katalogu www.
- aby logo się wczytało uploadujemy plik \*.raw przez webboard (settings/BOARD), nazwa pliku raw musi być identyczna z tą wyświetlaną na górze ekranu w playerze.
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
