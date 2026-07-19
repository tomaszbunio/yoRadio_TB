# yoRadio – Tomasz Bunio fork

Moje modyfikacje projektu yoRadio autorstwa [e2002](https://github.com/e2002/yoradio)
i [VaraiTamas](https://github.com/VaraiTamas/yoRadio).

## Moje zmiany: v8.8.1_TB

- Dodano obsługę ulubionych stacji FAV za pomocą pilota na ekranie Preset.
- Dodano import i eksport kopii pamięci FAV w edytorze playlisty WWW.
- Dodano pamiętanie ostatnio odtwarzanego utworu SD oraz opcjonalne wznowienie od zapisanej pozycji.
- Pozycja utworu SD i czas odtwarzania są zapisywane w EEPROM.
- Poprawiono obsługę pauzy, odświeżanie ekranu i dekodowanie okładek w trybie SD_PLAYER.
- Ograniczono przerwy w odtwarzaniu podczas ładowania panelu WWW.

### Pamięć ostatniego utworu SD

Zachowanie odtwarzacza SD można skonfigurować w pliku `myoptions.h`:

- `#define SD_REMEMBER_LAST_TRACK` – po ponownym uruchomieniu trybu SD wybiera ostatnio odtwarzany utwór. Po wyłączeniu tej opcji SD rozpoczyna od pierwszego utworu.
- `#define SD_REMEMBER_LAST_POSITION` – zapisuje również pozycję ostatniego utworu i wznawia odtwarzanie od tego miejsca. Numer utworu, pozycja pliku i czas są przechowywane w EEPROM, dlatego pozostają zapamiętane po restarcie lub odłączeniu zasilania.

Po wybraniu innego utworu zapamiętana pozycja zostaje skasowana. Pozwala to używać pamiętania ostatniego utworu bez wznawiania pozycji, a w przypadku audiobooków włączyć obie opcje.

### Obsługa FAV z pilota

Funkcja jest dostępna na ekranie Preset dla wyświetlacza ILI9488. Każda karta FAV zawiera osiem numerowanych pól pamięci.

1. Naciśnij `HOME`, aby otworzyć ekran Preset.
2. Klawiszami `PREV` i `NEXT` wybierz kartę FAV.
3. Klawiszami `UP` i `DOWN` wybierz na górnym pasku funkcję `Otwórz`, `Zapisz` albo `Usuń`. Aktywna funkcja jest podświetlona na niebiesko.
4. Naciśnij cyfrę od `1` do `8`, aby wskazać pole pamięci. Wybrane pole zostanie podświetlone:
   - jaśniejszym niebieskim dla funkcji `Otwórz`,
   - zielonym dla funkcji `Zapisz`,
   - czerwonym dla funkcji `Usuń`.
5. Naciśnij `PLAY`, aby wykonać wybraną operację.

Klawisz `BACK` anuluje wybór funkcji lub pola. Zapis umieszcza w wybranym polu aktualnie odtwarzaną stację radiową.

---

## Moje zmiany: v8.8_TB

- Sortowanie playlisty SD alfabetycznie zamiast kolejności FAT, co umożliwia poprawną numerację albumów.
- Oprogramowanie posiada wbudowane domyślne przypisania przycisków pilota (czytaj [konfiguracja_IR.md](konfiguracja_IR.md)) oraz obsługuje wybudzanie ESP32 z soft standby (`IR_PIN` GPIO 2).
- Dodano zmianę czcionki zegara dotykiem obszaru minut oraz umożliwiono przełączanie trybu SD/Radio dotykiem obszaru sekund zegara na ekranie PLAYER.
- Dodano dedykowany ekran SD_PLAYER dla odtwarzania z karty SD, z paskiem postępu i sterowaniem.
- Naprawiono auto-play.
- Obsługa NeoPixel i Clock przez WWW.
- Dodano pobieranie okładki z Last.fm, gdy nie uda się jej zdekodować z pliku MP3.
- Dodano wizualizację FFT do odtwarzacza dla trybu noTouch.
- Dodatkowo kilkadziesiąt optymalizacji w zakresie szybkości wyświetlania.

---

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
