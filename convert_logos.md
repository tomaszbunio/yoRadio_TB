# Konwerter logo stacji `scripts/convert_logos.py`

## W skrócie

Konwerter zamienia logo stacji z plików PNG na format RAW używany przez
yoRadio. Najprościej uruchomić `convert_logos.bat`, który przygotuje logo
i skopiuje właściwy zestaw (dopasowany do rozdzielczości wyświetlacza) do `data/www`.

## Konwersja logo na podstawie playlisty

Konwerter nie przetwarza automatycznie wszystkich obrazów z `logos_src`.
Odczytuje pierwszą kolumnę (nazwę stacji) z pliku:

```text
data/data/playlist.csv
```

Dla każdej stacji wybierane jest:

1. indywidualne logo, jeśli plik PNG istnieje w `logos_src`
2. logo zdefiniowane jako fallback,
3. `logo_default.png`, jeśli nie znaleziono innego dopasowania.

Jedno logo fallback może obsługiwać wiele stacji. Z tego powodu liczba
wygenerowanych plików RAW może być znacznie mniejsza niż liczba pozycji
w playliście.

Stacja bez indywidualnego PNG nie zatrzymuje konwersji. W podsumowaniu
zostanie wymieniona jako korzystająca z `logo_default.raw`.

## Jak dodać logo do stacji radiowej

1. Sprawdź dokładną nazwę stacji w pierwszej kolumnie pliku:

   ```text
   data/data/playlist.csv
   ```

2. Przygotuj logo w formacie PNG. Najlepiej użyć obrazu dobrej jakości,
   z zachowanymi marginesami wokół znaku stacji. Obraz nie musi mieć
   docelowych wymiarów — konwerter sam zachowa proporcje, przeskaluje go
   i wyśrodkuje na czarnym tle.

3. Nazwij plik PNG nazwą odpowiadającą nazwie stacji i umieść go w:

   ```text
   logos_src/
   ```

   Przykład:

   ```text
   nazwa stacji w playliście: Radio Złote Przeboje
   plik źródłowy:             logos_src/Radio Złote Przeboje.png
   plik 320×240:              logos_raw/320x240/radio_zlote_przeboje.raw
   plik 480×320:              logos_raw/480x320/radio_zlote_przeboje.raw
   ```

   Wielkość liter, polskie znaki, spacje i myślniki są obsługiwane przez
   normalizację opisaną niżej. Najbezpieczniej jednak zachować w nazwie PNG
   pełną nazwę stacji z playlisty.

4. Uruchom konwerter z głównego katalogu projektu:

   ```powershell
   convert_logos.bat
   ```

   lub:

   ```powershell
   python scripts/convert_logos.py
   ```

5. Sprawdź podsumowanie konwersji i listę stacji korzystających z
   `logo_default.raw`. Dodanej stacji nie powinno być na tej liście.
   Skrypt nie wyświetla osobnego licznika indywidualnych dopasowań.
   Odpowiedni plik `.raw` powinien znajdować się w katalogu wariantu
   `logos_raw`. Jeśli użyto `convert_logos.bat`, zestaw dla aktywnego
   wyświetlacza został już skopiowany do `data/www`, więc nowe logo powinno
   znajdować się również w tym katalogu. 

6. Zbuduj i wgraj system plików SPIFFS:

   ```powershell
   pio run -t uploadfs
   ```

   Samo `pio run` kompiluje firmware, ale nie wgrywa nowych plików logo.

7. Po uruchomieniu radia wybierz daną stację i sprawdź logo na ekranie.
   Jeśli nadal wyświetla się logo domyślne, porównaj nazwę stacji
   w playliście z nazwą PNG oraz sprawdź komunikaty konwertera.

Jeżeli nazwy nie da się poprawnie powiązać samą normalizacją, dodaj jawne
mapowanie w słowniku `SOURCE_STEM_ALIASES` w pliku
`scripts/convert_logos.py`. Nie zmieniaj ręcznie nazwy wygenerowanego pliku
RAW — radio wylicza ją z nazwy stacji według tych samych reguł.


## Szczegółowy opis

Skrypt `scripts/convert_logos.py` przygotowuje logo stacji radiowych do zapisania
w systemie plików SPIFFS yoRadio.

Źródłowe obrazy PNG są skalowane i zapisywane jako pliki RAW RGB565.
Domyślne uruchomienie generuje zestaw dla aktywnego wyświetlacza, natomiast
`convert_logos.bat` generuje oba obsługiwane warianty.

## Przepływ plików

```text
logos_src/*.png
        +
data/data/playlist.csv
        +
myoptions.h (DSP_MODEL)
        |
        v
scripts/convert_logos.py
        |
        v
logos_raw/320x240/*.raw
logos_raw/480x320/*.raw
        |
        +---- convert_logos.bat / --copy-to-www ----+
        |                                            |
        +---- scripts/prepare_spiffs_logos.py -------+
                                                     |
                                                     v
                                  data/www/*.raw (aktywny wariant)
        |
        v
PlatformIO buildfs / uploadfs
        |
        v
/www/*.raw w SPIFFS
```

## Wymagania

- Python 3,
- biblioteka Pillow.

Instalacja Pillow:

```powershell
pip install Pillow
```

## Uruchamianie

Z głównego katalogu projektu:

```powershell
python scripts/convert_logos.py
```

Można również użyć:

```powershell
convert_logos.bat
```

Plik `convert_logos.bat` generuje jednocześnie oba używane warianty:

```text
logos_raw/320x240
logos_raw/480x320
```

Następnie sprawdza pliki i kopiuje do `data/www` tylko wariant odpowiadający
aktywnemu `DSP_MODEL`.

Ten sam przebieg można uruchomić ręcznie:

```powershell
python scripts/convert_logos.py --all-resolutions --copy-to-www
```

Opcja `--copy-to-www`:

1. odczytuje aktywny `DSP_MODEL`,
2. wybiera odpowiadający mu katalog w `logos_raw`,
3. sprawdza obecność plików i `logo_default.raw`,
4. sprawdza rozmiar każdego pliku,
5. usuwa dotychczasowe `data/www/*.raw`,
6. kopiuje wybrany zestaw do `data/www`.

Bez opcji `--copy-to-www` konwerter nie zmienia zawartości `data/www`.

Po zakończeniu konwersji obraz SPIFFS można zbudować poleceniem:

```powershell
pio run -t buildfs
```

Wgranie samego systemu plików:

```powershell
pio run -t uploadfs
```

## Wybór rozmiaru logo

Skrypt odczytuje aktywną definicję `DSP_MODEL` z `myoptions.h`.

| Model wyświetlacza | Rozdzielczość LCD | Rozmiar logo |
|---|---:|---:|
| `DSP_ILI9341` | 320×240 | 72×48 |
| `DSP_ILI9486` | 480×320 | 120×90 |
| `DSP_ILI9488` | 480×320 | 120×90 |
| `DSP_ST7796` | 480×320 | 120×90 |

Domyślne uruchomienie skryptu generuje wariant dla aktywnego wyświetlacza.
Oba zestawy można wygenerować poleceniem:

```powershell
python scripts/convert_logos.py --all-resolutions
```

Można również przebudować tylko jeden zestaw:

```powershell
python scripts/convert_logos.py --resolution 320x240
python scripts/convert_logos.py --resolution 480x320
```


## Normalizacja nazw

Nazwa wynikowego pliku jest tworzona tak samo jak ścieżka w
`StationLogoWidget`:

- wielkie litery są zamieniane na małe,
- polskie znaki są transliterowane do ASCII,
- spacje i myślniki są zamieniane na `_`,
- pozostała interpunkcja jest pomijana,
- nazwa jest ograniczona do 22 znaków.

Przykłady:

```text
Polskie Radio Dwójka  -> polskie_radio_dwojka.raw
Radio Złote Przeboje  -> radio_zlote_przeboje.raw
RMF - Muzyka Klasyczna -> rmf_muzyka_klasyczna.raw
```

Kod `StationLogoWidget` celowo ogranicza nazwę do 22 znaków. Zapewnia to
krótką, przewidywalną ścieżkę:

```text
/www/ + 22 znaki + .raw
```

## Aliasy nazw źródłowych

Niektórych nazw PNG nie można jednoznacznie dopasować przez
samą normalizację. Skrypt posiada jawne aliasy:

```text
eska2.png                    -> radio_eska_2.raw
Radio_em.png                 -> radio_e_m.raw
Radio_Kielce_Folk_Radio.png  -> folk_radio.raw
Norda_FM.png                 -> radio_norda_fm.raw
```

Aliasy znajdują się w słowniku `SOURCE_STEM_ALIASES`.

## Fallbacki

Konwerter posiada własną listę fallbacków odpowiadającą konfiguracji
`STATION_LOGO_FALLBACKS` w `myoptions.h`. Obie listy trzeba aktualizować
razem. Pozwalają one użyć jednego logo dla grupy stacji, np.:

```text
RMF - 80s
RMF - Dance
RMF - Gold
```

mogą korzystać z:

```text
rmf_fm.raw
```

Analogicznie wspólne logo mogą wykorzystywać stacje z grup ZET, ESKA
i Antyradio.

## Skalowanie obrazu

Skrypt zachowuje proporcje źródłowego PNG:

1. obraz jest skalowany tak, aby zmieścił się w docelowym prostokącie,
2. brakujący obszar jest wypełniany czernią,
3. obraz jest wyśrodkowany,
4. piksele są zapisywane jako RGB565 little-endian.

Rozmiar pliku wynikowego:

```text
szerokość × wysokość × 2 bajty
```

Przykłady:

```text
72 × 48 × 2  = 6912 bajtów
120 × 90 × 2 = 21600 bajtów
```

## Czyszczenie starych RAW

Przed zapisaniem nowego zestawu konwerter usuwa stare pliki tylko z aktualnie
generowanego katalogu:

```text
logos_raw/320x240/*.raw
```

albo:

```text
logos_raw/480x320/*.raw
```

Czyszczenie następuje dopiero po:

- odczytaniu konfiguracji,
- odnalezieniu PNG,
- odczytaniu playlisty,
- sprawdzeniu normalizacji nazw,
- sprawdzeniu kolizji,
- potwierdzeniu obecności `logo_default.png`,
- poprawnym załadowaniu Pillow.

Błąd na jednym z tych etapów nie usuwa istniejącego zestawu RAW.

Bez opcji `--copy-to-www` konwerter nie czyści `data/www`. Plik
`convert_logos.bat` używa tej opcji, dlatego po poprawnej walidacji zastępuje
pliki `data/www/*.raw` zestawem dla aktywnego wyświetlacza.

Błąd konwersji pojedynczego PNG jest wypisywany przy jego nazwie, ale nie
zatrzymuje przetwarzania pozostałych obrazów. Walidacja przed kopiowaniem
sprawdza obecność `logo_default.raw` i rozmiary istniejących plików, ale nie
sprawdza kompletności całego zestawu.

## Automatyczny wybór zestawu do SPIFFS

Pre-skrypt `scripts/prepare_spiffs_logos.py` jest podłączony w
`platformio.ini`. Uruchamia się przed tworzeniem `spiffs.bin`.

Skrypt:

1. odczytuje aktywny `DSP_MODEL` z `myoptions.h`,
2. wybiera `logos_raw/320x240` albo `logos_raw/480x320`,
3. sprawdza obecność plików i `logo_default.raw`,
4. sprawdza rozmiar każdego pliku RAW,
5. dopiero po poprawnej kontroli usuwa `data/www/*.raw`,
6. kopiuje do `data/www` wyłącznie wybrany zestaw,
7. pozwala PlatformIO zbudować obraz SPIFFS.

Nieudana walidacja nie usuwa działających plików z `data/www`.

## Kontrola kolizji

Jeśli dwa różne pliki PNG po normalizacji utworzyłyby tę samą nazwę RAW,
konwerter przerwie pracę przed usunięciem istniejących plików.

Przykład potencjalnej kolizji:

```text
Radio-Test.png
Radio Test.png
```

Obie nazwy prowadziłyby do:

```text
radio_test.raw
```

## Podsumowanie konwersji

Przed konwersją skrypt wyświetla podsumowanie podobne do poniższego
(liczby zależą od aktualnej playlisty i zawartości `logos_src`):

```text
Playlista: <liczba stacji>
wybrano PNG: <liczba plików>
fallbacki: <liczba dopasowań>
logo domyślne: <liczba stacji>
pominięto PNG: <liczba plików>
```

Na końcu wyświetlana jest liczba poprawnie wygenerowanych plików oraz ich
łączny rozmiar.

## Ważne

Standardowe polecenie:

```powershell
pio run
```

buduje firmware, ale nie generuje automatycznie nowych plików RAW i nie
buduje obrazu SPIFFS. `buildfs` oraz `uploadfs` automatycznie wybierają
istniejący zestaw odpowiedni dla `DSP_MODEL`.

Po zmianie playlisty albo źródłowych PNG należy ponownie uruchomić:

```powershell
convert_logos.bat
```

a następnie `buildfs` lub `uploadfs`.
