#!/usr/bin/env python3
"""Raportuje lub przenosi PNG logo nieużywane przez bieżącą playlistę."""

import argparse
import shutil
from pathlib import Path

from convert_logos import output_logo_stem, station_logo_stem

ROOT = Path(__file__).resolve().parent.parent
PLAYLIST = ROOT / "data" / "data" / "playlist.csv"
SOURCE_DIR = ROOT / "logos_src"
UNUSED_DIR = ROOT / "logos_src_unused"

FALLBACKS = (
    ("rmf", "rmf_fm"),
    ("zet", "radio_zet"),
    ("antyradio", "antyradio"),
    ("eska", "radio_eska"),
    ("open fm", "open_fm"),
    ("e m", "radio_em"),
)

PROTECTED_OUTPUT_STEMS = {"logo_default", "bootlogo"}


def read_station_names():
    return [
        line.split("\t", 1)[0]
        for line in PLAYLIST.read_text(encoding="utf-8-sig").splitlines()
        if line.strip()
    ]


def build_plan():
    sources = {}
    collisions = []
    for png in sorted(SOURCE_DIR.glob("*.[pP][nN][gG]")):
        output_stem = output_logo_stem(png.stem)
        if output_stem in sources:
            collisions.append((output_stem, sources[output_stem], png))
        else:
            sources[output_stem] = png

    if collisions:
        details = "\n".join(
            f"  {first.name} + {second.name} -> {stem}.raw"
            for stem, first, second in collisions
        )
        raise RuntimeError(f"Kolizje nazw wynikowych:\n{details}")

    used = set(PROTECTED_OUTPUT_STEMS)
    missing = []
    for station in read_station_names():
        station_stem = station_logo_stem(station)
        if station_stem in sources:
            used.add(station_stem)
            continue

        fallback = next(
            (
                file_stem
                for prefix, file_stem in FALLBACKS
                if station_stem.startswith(prefix) and file_stem in sources
            ),
            None,
        )
        if fallback:
            used.add(fallback)
        else:
            missing.append((station, station_stem))

    unused = [png for stem, png in sources.items() if stem not in used]
    return sources, used, sorted(unused), missing


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--move",
        action="store_true",
        help="przenieś nieużywane PNG do logos_src_unused",
    )
    args = parser.parse_args()

    sources, used, unused, missing = build_plan()
    print(
        f"Playlista: {len(read_station_names())}, PNG: {len(sources)}, "
        f"używane: {len(used & set(sources))}, nieużywane: {len(unused)}"
    )

    if missing:
        print("PRZERWANO: stacje bez logo ani fallbacku:")
        for station, stem in missing:
            print(f"  {station} -> {stem}.raw")
        raise SystemExit(2)

    for png in unused:
        print(f"  {png.name}")

    if not args.move:
        print("Tryb kontrolny: niczego nie przeniesiono.")
        return

    source_root = SOURCE_DIR.resolve()
    unused_root = UNUSED_DIR.resolve()
    source_root.relative_to(ROOT)
    unused_root.relative_to(ROOT)
    UNUSED_DIR.mkdir(exist_ok=True)

    existing = [UNUSED_DIR / png.name for png in unused if (UNUSED_DIR / png.name).exists()]
    if existing:
        print("PRZERWANO: pliki docelowe już istnieją:")
        for path in existing:
            print(f"  {path.name}")
        raise SystemExit(3)

    for png in unused:
        png.resolve().relative_to(source_root)
        shutil.move(str(png), str(UNUSED_DIR / png.name))

    print(f"Przeniesiono: {len(unused)} plików do {UNUSED_DIR}")


if __name__ == "__main__":
    main()
