Import("env")
import shutil
from pathlib import Path

src = Path("locale/glcdfont/PL/glcdfont.c")
dst = Path(".pio/libdeps") / env["PIOENV"] / "Adafruit GFX Library" / "glcdfont.c"

if not src.exists():
    print(f"[glcdfont] BŁĄD: nie znaleziono {src}")
elif not dst.parent.exists():
    print(f"[glcdfont] POMINIĘTO: katalog docelowy nie istnieje (biblioteka jeszcze nie pobrana)")
else:
    shutil.copy2(src, dst)
    print(f"[glcdfont] Skopiowano {src} -> {dst}")
