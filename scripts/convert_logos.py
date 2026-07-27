#!/usr/bin/env python3
"""
Konwerter logo stacji radiowych PNG -> RGB565 binary (.raw)
Użycie: python scripts/convert_logos.py

Skrypt wybiera obrazy PNG potrzebne przez playlistę, konwertuje je
do formatu raw RGB565 (little-endian, 2 bajty/piksel) i zapisuje
w katalogach logos_raw nazwanych rozdzielczością LCD.

Format wynikowy: WIDTH * HEIGHT * 2 bajtów
  Każdy piksel: uint16_t little-endian, RGB565
  Driver ILI9486_SPI zamienia bajty przed wysłaniem przez SPI.

Wymagania: pip install Pillow
"""

import argparse
import re
import shutil
import struct
import sys
from pathlib import Path

Image = None

# Wymiary docelowe (muszą być zgodne z myoptions.h)
TARGET_W = 0
TARGET_H = 0

PROJECT_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_DIR / "logos_src"
OUT_ROOT = PROJECT_DIR / "logos_raw"
OPTIONS_FILE = PROJECT_DIR / "myoptions.h"
PLAYLIST_FILE = PROJECT_DIR / "data" / "data" / "playlist.csv"
SPIFFS_WWW_DIR = PROJECT_DIR / "data" / "www"

DISPLAY_LOGO_SIZES = {
    "DSP_ILI9341": (72, 48),
    "DSP_ILI9486": (120, 90),
    "DSP_ILI9488": (120, 90),
    "DSP_ST7796":  (120, 90),
}

DISPLAY_RESOLUTIONS = {
    "DSP_ILI9341": "320x240",
    "DSP_ILI9486": "480x320",
    "DSP_ILI9488": "480x320",
    "DSP_ST7796":  "480x320",
}

LCD_DISPLAY_MODELS = {
    "LCD_ILI9341": "DSP_ILI9341",
    "LCD_ILI9488": "DSP_ILI9488",
    "LCD_ST7796":  "DSP_ST7796",
}

RESOLUTION_LOGO_SIZES = {
    "320x240": (72, 48),
    "480x320": (120, 90),
}

PL_TRANSLITERATION = str.maketrans({
    "Ą": "A", "ą": "a", "Ć": "C", "ć": "c", "Ę": "E", "ę": "e",
    "Ł": "L", "ł": "l", "Ń": "N", "ń": "n", "Ó": "O", "ó": "o",
    "Ś": "S", "ś": "s", "Ź": "Z", "ź": "z", "Ż": "Z", "ż": "z",
})


def station_logo_stem(name: str) -> str:
    """Normalizacja zgodna z StationLogoWidget::_buildStem()."""
    out = []
    for char in name.translate(PL_TRANSLITERATION):
        if char in " -":
            if not out or out[-1] != "_":
                out.append("_")
        elif char.isascii() and (char.isalnum() or char == "_"):
            out.append(char.lower())
        if len(out) >= 22:
            break
    return "".join(out)

SOURCE_STEM_ALIASES = {
    "eska2": "radio_eska_2",
    "radio_em": "radio_e_m",
    "radio_kielce_folk_radi": "folk_radio",
    "norda_fm": "radio_norda_fm",
}

STATION_LOGO_FALLBACKS = (
    ("rmf", "rmf_fm"),
    ("zet", "radio_zet"),
    ("antyradio", "antyradio"),
    ("eska", "radio_eska"),
    ("open_fm", "open_fm"),
    ("e_m", "radio_e_m"),
)


def output_logo_stem(source_name: str) -> str:
    stem = station_logo_stem(source_name)
    return SOURCE_STEM_ALIASES.get(stem, stem)


def select_playlist_logos(png_files):
    """Wybierz tylko PNG potrzebne przez playlistę, fallbacki i logo domyślne."""
    output_names = {}
    collisions = []
    for png in png_files:
        stem = output_logo_stem(png.stem)
        if not stem:
            collisions.append(f"{png.name}: nazwa po normalizacji jest pusta")
        elif stem in output_names:
            collisions.append(f"{output_names[stem].name} oraz {png.name} -> {stem}.raw")
        else:
            output_names[stem] = png

    if collisions:
        raise ValueError("kolizje nazw plikow wynikowych:\n  " + "\n  ".join(collisions))

    station_names = [
        line.split("\t", 1)[0]
        for line in PLAYLIST_FILE.read_text(encoding="utf-8-sig").splitlines()
        if line.strip()
    ]
    required = set()
    missing = []
    fallback_count = 0
    for station_name in station_names:
        station_stem = station_logo_stem(station_name)
        if station_stem in output_names:
            required.add(station_stem)
            continue

        fallback = next(
            (
                file_stem
                for prefix, file_stem in STATION_LOGO_FALLBACKS
                if station_stem.startswith(prefix) and file_stem in output_names
            ),
            None,
        )
        if fallback:
            required.add(fallback)
            fallback_count += 1
        else:
            missing.append(f"{station_name} -> {station_stem}.raw")

    if "logo_default" in output_names:
        required.add("logo_default")
    else:
        raise ValueError("brak wymaganego pliku logo_default.png")

    selected = [output_names[stem] for stem in sorted(required)]
    return selected, len(station_names), fallback_count, len(missing), missing, len(png_files) - len(selected)


def read_logo_size(options_path: Path):
    """Odczytaj rozmiar logo przypisany do aktywnego DSP_MODEL."""
    text = options_path.read_text(encoding="utf-8")

    active_lcds = re.findall(
        r"^\s*#define\s+(LCD_[A-Z0-9_]+)\s*(?://.*)?$",
        text,
        re.MULTILINE,
    )
    known_lcds = [lcd for lcd in active_lcds if lcd in LCD_DISPLAY_MODELS]
    if len(known_lcds) > 1:
        raise ValueError("wybrano wiecej niz jeden typ LCD: " + ", ".join(known_lcds))

    if known_lcds:
        model = LCD_DISPLAY_MODELS[known_lcds[0]]
    else:
        models = re.findall(
            r"^\s*#define\s+DSP_MODEL\s+(DSP_[A-Z0-9_]+)\s*(?://.*)?$",
            text,
            re.MULTILINE,
        )
        if len(models) != 1:
            raise ValueError(
                "nie mozna jednoznacznie ustalic aktywnego LCD/DSP_MODEL w myoptions.h"
            )
        model = models[0]

    if model not in DISPLAY_LOGO_SIZES or model not in DISPLAY_RESOLUTIONS:
        supported = ", ".join(DISPLAY_LOGO_SIZES)
        raise ValueError(f"brak konfiguracji logo dla {model}; obslugiwane: {supported}")

    width, height = DISPLAY_LOGO_SIZES[model]
    return width, height, model, DISPLAY_RESOLUTIONS[model]


def png_to_rgb565(png_path: Path, bin_path: Path):
    img = Image.open(png_path).convert("RGB")
    src_w, src_h = img.size

    # Skalowanie z zachowaniem proporcji (letterbox – czarne pasy)
    scale = min(TARGET_W / src_w, TARGET_H / src_h)
    new_w = int(src_w * scale)
    new_h = int(src_h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)

    # Nowe płótno 120×90 wypełnione czernią
    canvas = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    # Wyśrodkowanie obrazu na płótnie
    offset_x = (TARGET_W - new_w) // 2
    offset_y = (TARGET_H - new_h) // 2
    canvas.paste(img, (offset_x, offset_y))

    if src_w != TARGET_W or src_h != TARGET_H:
        print(f"  {src_w}x{src_h} -> {new_w}x{new_h} (pasy: x={offset_x}, y={offset_y})")

    pixels = list(canvas.getdata())
    with open(bin_path, "wb") as f:
        for r, g, b in pixels:
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            f.write(struct.pack("<H", rgb565))  # little-endian

    kb = bin_path.stat().st_size / 1024
    print(f"  -> {bin_path.name} ({kb:.1f} KB)")

def main():
    global TARGET_W, TARGET_H, Image
    parser = argparse.ArgumentParser(description="Konwersja logo stacji PNG do RGB565 RAW")
    parser.add_argument(
        "--copy-to-www",
        action="store_true",
        help="skopiuj wariant aktywnego DSP_MODEL do data/www",
    )
    output_group = parser.add_mutually_exclusive_group()
    output_group.add_argument(
        "--resolution",
        choices=sorted(RESOLUTION_LOGO_SIZES),
        help="wygeneruj wariant dla podanej rozdzielczosci LCD",
    )
    output_group.add_argument(
        "--all-resolutions",
        action="store_true",
        help="wygeneruj warianty dla wszystkich obslugiwanych rozdzielczosci LCD",
    )
    args = parser.parse_args()

    try:
        if args.resolution:
            width, height = RESOLUTION_LOGO_SIZES[args.resolution]
            profiles = [
                (args.resolution, width, height, f"wariant LCD {args.resolution}")
            ]
        elif args.all_resolutions:
            profiles = [
                (resolution, size[0], size[1], f"wariant LCD {resolution}")
                for resolution, size in sorted(RESOLUTION_LOGO_SIZES.items())
            ]
        else:
            width, height, size_source, resolution = read_logo_size(OPTIONS_FILE)
            profiles = [(resolution, width, height, size_source)]
    except (OSError, UnicodeError, ValueError) as error:
        print(f"BLAD konfiguracji rozmiaru: {error}")
        sys.exit(2)

    print(f"Szukam PNG w:  {DATA_DIR.resolve()}")
    print(f"Magazyn .raw:  {OUT_ROOT.resolve()}\n")
    OUT_ROOT.mkdir(exist_ok=True)

    all_png_files = sorted(DATA_DIR.glob("*.[pP][nN][gG]"))
    if not all_png_files:
        print(f"Brak plików .png w {DATA_DIR}")
        return

    try:
        (
            png_files,
            station_count,
            fallback_count,
            default_count,
            default_stations,
            skipped_count,
        ) = select_playlist_logos(all_png_files)
    except (OSError, UnicodeError, ValueError) as error:
        print(f"BLAD wyboru logo z playlisty: {error}")
        sys.exit(3)

    print(
        f"Playlista: {station_count} stacji, wybrano PNG: {len(png_files)}, "
        f"fallbacki: {fallback_count}, logo domyslne: {default_count}, "
        f"pominieto PNG: {skipped_count}"
    )
    if default_stations:
        print("Stacje korzystajace z logo_default.raw:")
        for station in default_stations:
            print(f"  {station}")
    try:
        from PIL import Image as PillowImage
        Image = PillowImage
    except ImportError:
        print("Brak biblioteki Pillow. Zainstaluj: pip install Pillow")
        sys.exit(1)

    total_ok = 0
    for profile, width, height, size_source in profiles:
        TARGET_W, TARGET_H = width, height
        out_dir = OUT_ROOT / profile
        out_dir.mkdir(parents=True, exist_ok=True)

        print(f"\n=== LCD {profile}: logo {TARGET_W}x{TARGET_H} ({size_source}) ===")
        print(f"Zapisuje .raw: {out_dir.resolve()}")

        raw_files = list(out_dir.glob("*.[rR][aA][wW]"))
        for raw_file in raw_files:
            raw_file.unlink()
        print(f"Usunieto starych plikow .raw z wariantu: {len(raw_files)}")

        ok = 0
        for png in png_files:
            bin_path = out_dir / f"{output_logo_stem(png.stem)}.raw"
            print(f"{png.name}")
            try:
                png_to_rgb565(png, bin_path)
                ok += 1
            except Exception as error:
                print(f"  BLAD: {error}")

        total_ok += ok
        total_kb = sum(p.stat().st_size for p in out_dir.glob("*.raw")) / 1024
        print(
            f"Gotowe dla {profile}: {ok}/{len(png_files)} plikow, "
            f"{total_kb:.0f} KB"
        )

    print(
        f"\nWygenerowano lacznie: {total_ok} plikow "
        f"w {len(profiles)} wariantach"
    )

    if args.copy_to_www:
        try:
            width, height, model, resolution = read_logo_size(OPTIONS_FILE)
            source_dir = OUT_ROOT / resolution
            raw_files = sorted(source_dir.glob("*.raw"))
            expected_size = width * height * 2

            if not raw_files:
                raise ValueError(f"brak plikow RAW w {source_dir}")
            if not (source_dir / "logo_default.raw").is_file():
                raise ValueError(f"brak {source_dir / 'logo_default.raw'}")

            invalid = [
                raw_file.name
                for raw_file in raw_files
                if raw_file.stat().st_size != expected_size
            ]
            if invalid:
                raise ValueError(
                    f"pliki o zlym rozmiarze (oczekiwano {expected_size} B): "
                    + ", ".join(invalid[:5])
                )

            SPIFFS_WWW_DIR.mkdir(parents=True, exist_ok=True)
            for raw_file in SPIFFS_WWW_DIR.glob("*.raw"):
                raw_file.unlink()
            for raw_file in raw_files:
                shutil.copy2(raw_file, SPIFFS_WWW_DIR / raw_file.name)

            copied_files = sorted(SPIFFS_WWW_DIR.glob("*.raw"))
            if len(copied_files) != len(raw_files) or any(
                raw_file.stat().st_size != expected_size for raw_file in copied_files
            ):
                raise ValueError("kontrola plikow docelowych data/www nie powiodla sie")

            print(
                f"Skopiowano i sprawdzono {len(raw_files)} plikow dla {model}, "
                f"wariant {resolution} ({width}x{height}, {expected_size} B/plik) "
                f"do {SPIFFS_WWW_DIR.resolve()}"
            )
        except (OSError, UnicodeError, ValueError) as error:
            print(f"BLAD kopiowania do data/www: {error}")
            sys.exit(4)

if __name__ == "__main__":
    main()
