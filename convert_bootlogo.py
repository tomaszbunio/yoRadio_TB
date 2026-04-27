#!/usr/bin/env python3
"""
Konwerter boot logo PNG -> nagłówki C (RGB565)
Użycie: python convert_bootlogo.py

Wejście:  logos_src/bootlogo.png
Wyjście:  src/displays/fonts/bootlogo{W}x{H}.h  (jeden plik na rozmiar)

Rozmiary dopasowane do karteczki flip clock:
  80x80  -> ILI9488, ST7796  (480x320)
  54x54  -> ILI9341, ST7789  (320x240)
  62x40  -> ST7789 mały      (bez zmian, generowany osobno)
  21x32  -> SSD1322 OLED     (bez zmian, generowany osobno)

Wymagania: pip install Pillow
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Brak biblioteki Pillow. Zainstaluj: pip install Pillow")
    sys.exit(1)

# Rozmiary do wygenerowania: (szerokość, wysokość)
SIZES = [
    (80, 80),   # ILI9488, ST7796  (480x320)
    (54, 54),   # ILI9341, ST7789  (320x240)
]

SRC      = Path(__file__).parent / "logos_src" / "bootlogo.png"
FONT_DIR = Path(__file__).parent / "src" / "displays" / "fonts"

def resize_letterbox(img: Image.Image, w: int, h: int) -> Image.Image:
    scale = min(w / img.width, h / img.height)
    new_w = int(img.width * scale)
    new_h = int(img.height * scale)
    img = img.resize((new_w, new_h), Image.LANCZOS)
    canvas = Image.new("RGB", (w, h), (0, 0, 0))
    canvas.paste(img, ((w - new_w) // 2, (h - new_h) // 2))
    return canvas

def to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert(img: Image.Image, w: int, h: int):
    out = FONT_DIR / f"bootlogo{w}x{h}.h"
    canvas = resize_letterbox(img, w, h)
    pixels = [canvas.getpixel((x, y)) for y in range(h) for x in range(w)]
    values = [to_rgb565(r, g, b) for r, g, b in pixels]
    guard  = f"bootlogo{w}x{h}_h"

    with open(out, "w", encoding="utf-8") as f:
        f.write(f"#ifndef {guard}\n")
        f.write(f"#define {guard}\n\n")
        f.write(f"#include <stdint.h>\n")
        f.write(f"#define LOGO_WIDTH    {w}\n")
        f.write(f"#define LOGO_HEIGHT   {h}\n\n")
        f.write(f"static const uint16_t logo[{w * h}] PROGMEM = {{\n")
        for i in range(0, len(values), w):
            line = ", ".join(f"0x{v:04X}" for v in values[i:i + w])
            f.write(f"    {line},\n")
        f.write("};\n\n")
        f.write(f"#endif // {guard}\n")

    print(f"  -> bootlogo{w}x{h}.h  ({w * h * 2 / 1024:.1f} KB)")

def main():
    if not SRC.exists():
        print(f"Nie znaleziono pliku: {SRC}\nUmieść plik bootlogo.png w katalogu logos_src/")
        sys.exit(1)

    img = Image.open(SRC).convert("RGB")
    print(f"Wejście: {SRC.name} ({img.width}x{img.height})\n")

    for w, h in SIZES:
        convert(img, w, h)

    print("\nGotowe.")

if __name__ == "__main__":
    main()
    try:
        input("\nNaciśnij Enter aby zamknąć...")
    except EOFError:
        pass
