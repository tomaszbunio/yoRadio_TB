#!/usr/bin/env python3
"""
Konwerter logo stacji radiowych PNG -> RGB565 binary (.raw)
Użycie: python convert_logos.py

Skrypt konwertuje wszystkie pliki .png z katalogu 'data/'
do formatu raw RGB565 (little-endian, 2 bajty/piksel)
i zapisuje je jako .raw w tym samym katalogu.

Format wynikowy: WIDTH * HEIGHT * 2 bajtów
  Każdy piksel: uint16_t little-endian, RGB565
  Driver ILI9486_SPI zamienia bajty przed wysłaniem przez SPI.

Wymagania: pip install Pillow
"""

import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Brak biblioteki Pillow. Zainstaluj: pip install Pillow")
    sys.exit(1)

# Wymiary docelowe (muszą być zgodne z myoptions.h)
TARGET_W = 160
TARGET_H = 120

DATA_DIR = Path(__file__).parent / "data"

def png_to_rgb565(png_path: Path, bin_path: Path):
    img = Image.open(png_path).convert("RGB")
    src_w, src_h = img.size

    # Skalowanie z zachowaniem proporcji (letterbox – czarne pasy)
    scale = min(TARGET_W / src_w, TARGET_H / src_h)
    new_w = int(src_w * scale)
    new_h = int(src_h * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)

    # Nowe płótno 160×120 wypełnione czernią
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
    print(f"Szukam w: {DATA_DIR.resolve()}")
    png_files = sorted(DATA_DIR.glob("*.png"))
    if not png_files:
        print(f"Brak plików .png w {DATA_DIR}")
        return

    print(f"Konwersja {len(png_files)} plików PNG -> RGB565 binary ({TARGET_W}x{TARGET_H})")
    print(f"Katalog: {DATA_DIR}\n")

    ok = 0
    for png in png_files:
        bin_path = png.with_suffix(".raw")
        print(f"{png.name}")
        try:
            png_to_rgb565(png, bin_path)
            ok += 1
        except Exception as e:
            print(f"  BŁĄD: {e}")

    print(f"\nGotowe: {ok}/{len(png_files)} plików")
    total_kb = sum(p.stat().st_size for p in DATA_DIR.glob("*.raw")) / 1024
    print(f"Łączny rozmiar .raw: {total_kb:.0f} KB")

if __name__ == "__main__":
    main()
