# Konwerter Boot Logo

Konwertuje plik PNG na kolorowe nagłówki C w formacie RGB565, używane jako
logo ekranu startowego yoRadio.

## Wymagania

- Python 3.x
- biblioteka Pillow:

```
pip install Pillow
```

## Użycie

1. Umieść własne logo w pliku:

```
logos_src/bootlogo.png
```

2. Uruchom `convert_bootlogo.bat` albo wykonaj:

```
python convert_bootlogo.py
```

3. Skompiluj i wgraj firmware.

Plik wejściowy może mieć dowolny rozmiar i proporcje. Obraz zostanie
przeskalowany z zachowaniem proporcji i umieszczony na czarnym tle.

## Generowany plik dla ILI9488

Logo używane przez oprogramowanie z wyświetlaczem ILI9488 jest zapisywane jako:

```
src/displays/fonts/bootlogo80x80.h
```

## Pozycja logo

Firmware automatycznie centruje bootlogo poziomo:

```
(width() - LOGO_WIDTH) / 2
```

Pozycja pionowa nie jest wyliczana automatycznie. Dla ILI9488 określa ją
`bootLogoTop` w pliku:

```
src/displays/conf/displayILI9488conf.h
```

```
#define bootLogoTop 110
```

Wartość `110` oznacza pozycję górnej krawędzi logo na osi Y.

## Format wygenerowanego nagłówka

Każdy wygenerowany plik zawiera:

```
#define LOGO_WIDTH  ...
#define LOGO_HEIGHT ...
static const uint16_t logo[...] PROGMEM = { ... };
```

Kolory są zapisane jako RGB565. Kolorowe logo jest rysowane przez
`drawRGBBitmap()`.
