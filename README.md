# yoRadio – Tomasz Bunio fork

Moje modyfikacje projektu yoRadio autorstwa [e2002](https://github.com/e2002/yoradio)
i [VaraiTamas](https://github.com/VaraiTamas/yoRadio).

## Moje zmiany: v8.7.1_TB

- poprawienie błędów z v8.7
- możliwość zmiany bootlogo, czytaj [convert_bootlogo.md](convert_bootlogo.md)

---

## Moje zmiany: v0.8.7_TB - testowałem tylko na lcd ILI9488.

- Flip clock
- logo stacji radiowych zamiast pogody i bitrate
- automatyczna zmiana czasu z wykorzystaniem TIMEZONE_POSIX
- upłynnienie scrollingu title (co 3px zamiast co 7px).

### Konwerter logo stacji (`convert_logos.bat`)

- konwertuje wszystkie pliki `.png` z katalogu `data/` do binarnego formatu RGB565 (little-endian, 120×90 px)
- pliki `.raw` zapisywane są bezpośrednio do `data/www/`
- aby logo się wczytało, uploadujemy pliki `.raw` przez webboard (Settings → BOARD); nazwa pliku musi być identyczna z nazwą stacji wyświetlaną na górze ekranu
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
