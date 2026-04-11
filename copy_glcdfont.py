Import("env")
import shutil
from pathlib import Path

src = Path("locale/glcdfont/PL/glcdfont.c")
dst = Path(".pio/libdeps") / env["PIOENV"] / "Adafruit GFX Library" / "glcdfont.c"

if src.exists():
    shutil.copy2(src, dst)
    print(f"[glcdfont] Skopiowano {src} -> {dst}")
else:
    print(f"[glcdfont] BŁĄD: nie znaleziono {src}")
