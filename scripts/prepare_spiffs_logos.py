"""Wybiera zestaw logo RAW do obrazu SPIFFS na podstawie DSP_MODEL."""

Import("env")

import re
import shutil
from pathlib import Path


PROJECT_DIR = Path(env["PROJECT_DIR"])
OPTIONS_FILE = PROJECT_DIR / "myoptions.h"
VARIANTS_DIR = PROJECT_DIR / "logos_raw"
SPIFFS_WWW_DIR = PROJECT_DIR / "data" / "www"

DISPLAY_VARIANTS = {
    "DSP_ILI9341": ("320x240", 72, 48),
    "DSP_ILI9486": ("480x320", 120, 90),
    "DSP_ILI9488": ("480x320", 120, 90),
    "DSP_ST7796":  ("480x320", 120, 90),
}


def active_display_model():
    text = OPTIONS_FILE.read_text(encoding="utf-8")
    match = re.search(
        r"^\s*#define\s+DSP_MODEL\s+(DSP_[A-Z0-9_]+)\s*(?://.*)?$",
        text,
        re.MULTILINE,
    )
    if not match:
        raise RuntimeError("nie znaleziono aktywnego DSP_MODEL w myoptions.h")
    return match.group(1)


def prepare_spiffs_logos(source, target, env):
    model = active_display_model()
    if model not in DISPLAY_VARIANTS:
        supported = ", ".join(DISPLAY_VARIANTS)
        raise RuntimeError(
            f"[SPIFFS logos] brak wariantu dla {model}; obslugiwane: {supported}"
        )

    resolution, logo_w, logo_h = DISPLAY_VARIANTS[model]
    source_dir = VARIANTS_DIR / resolution
    raw_files = sorted(source_dir.glob("*.raw"))
    expected_size = logo_w * logo_h * 2

    if not raw_files:
        raise RuntimeError(
            f"[SPIFFS logos] brak plikow RAW w {source_dir}. "
            "Uruchom: python scripts/convert_logos.py --all-resolutions"
        )
    if not (source_dir / "logo_default.raw").is_file():
        raise RuntimeError(f"[SPIFFS logos] brak {source_dir / 'logo_default.raw'}")

    invalid = [
        raw_file.name
        for raw_file in raw_files
        if raw_file.stat().st_size != expected_size
    ]
    if invalid:
        preview = ", ".join(invalid[:5])
        raise RuntimeError(
            f"[SPIFFS logos] wariant {resolution} ma pliki o zlym rozmiarze "
            f"(oczekiwano {expected_size} B): {preview}"
        )

    SPIFFS_WWW_DIR.mkdir(parents=True, exist_ok=True)
    old_raw_files = list(SPIFFS_WWW_DIR.glob("*.raw"))
    for raw_file in old_raw_files:
        raw_file.unlink()
    for raw_file in raw_files:
        shutil.copy2(raw_file, SPIFFS_WWW_DIR / raw_file.name)

    print(
        f"[SPIFFS logos] {model} -> {resolution}: "
        f"skopiowano {len(raw_files)} plikow {logo_w}x{logo_h} do data/www"
    )


SPIFFS_IMAGE = "$BUILD_DIR/spiffs.bin"
variant_inputs = [str(path) for path in VARIANTS_DIR.glob("*/*.raw")]
env.AddPreAction(SPIFFS_IMAGE, prepare_spiffs_logos)
env.Depends(
    SPIFFS_IMAGE,
    [
        str(OPTIONS_FILE),
        str(PROJECT_DIR / "scripts" / "prepare_spiffs_logos.py"),
        *variant_inputs,
    ],
)
