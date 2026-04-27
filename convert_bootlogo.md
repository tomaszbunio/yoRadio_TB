# Konwerter Boot Logo

Konwertuje plik PNG na nagłówki C z danymi RGB565, gotowe do wgrania jako ekran startowy yoRadio.

## Wymagania

- Python 3.x
- Biblioteka Pillow: `pip install Pillow`

## Użycie

1. Umieść swoje logo jako `logos_src/bootlogo.png`
   - dowolny rozmiar i proporcje — obraz zostanie automatycznie przeskalowany (letterbox, tło czarne)
2. Uruchom `convert_bootlogo.bat` (dwuklik) lub w terminalu:
   ```
   python convert_bootlogo.py
   ```
3. Skompiluj i wgraj firmware — logo pojawi się przy starcie urządzenia.

## Pliki wyjściowe

| Plik | Wyświetlacz |
|------|-------------|
| `src/displays/fonts/bootlogo80x80.h` | ILI9488, ST7796 (480×320) |
| `src/displays/fonts/bootlogo54x54.h` | ILI9341, ST7789 (320×240) |

## Uwagi

- Plik wejściowy musi się nazywać dokładnie `bootlogo.png` i leżeć w katalogu `logos_src/`
- Logo jest centrowane; jeśli proporcje nie są kwadratowe, boki uzupełniane są czernią
- Wygenerowane pliki `.h` są nadpisywane przy każdym uruchomieniu
