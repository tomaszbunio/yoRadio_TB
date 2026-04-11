#!/usr/bin/env python3
"""
Porównuje stacje z playlisty z plikami .raw i pokazuje które można usunąć.
"""
import unicodedata
import os

# === Stacje z polishplaylist.csv ===
stations = [
    "Zet Same Hity", "Polskie Radio Jedynka", "Polskie Radio Dwójka",
    "Polskie Radio Trójka", "Polskie Radio Czwórka", "RMF FM", "Radio ZET",
    "Radio Plus", "Radio Złote Przeboje", "Radio ESKA", "Radio Kraków",
    "Radio Vox FM", "RMF Classic", "Antyradio", "Radio ESKA 2",
    "Meloradio FM", "Radio Wnet", "Radio Maryja", "Polskie Radio Chopin",
    "Polskie Radio 24", "Polskie Radio Kierowców", "RMF - Maxxx",
    "Radio Tok FM", "Radio Pogoda", "Radio 357", "Radio Nowy Świat",
    "Rock Radio Polska", "RMF - 50s", "RMF - 70s", "RMF - 70s Disco",
    "RMF - 80s", "RMF - 80s Disco", "RMF - 90s", "RMF - 90s Dance",
    "RMF - 2000s", "RMF - 2010s", "RMF - 35 LAT", "RMF - Ballady",
    "RMF - Beatle Mania", "RMF - Chillout", "RMF - Dance", "RMF - Dance RnB",
    "RMF - Fitness", "RMF - Fitness Rock", "RMF - Francais", "RMF - Vibe",
    "RMF - Game Music", "RMF - Gold", "RMF - Grunge", "RMF - Hard And Heavy",
    "RMF - Hip Hop", "RMF - Hot New", "RMF - Kolędy", "RMF - Latino",
    "RMF - Love", "RMF - Muzyka Klasyczna", "RMF - Niezapomniane Przeboje",
    "RMF - Party", "RMF - Piosenka Filmowa", "RMF - Piosenka Literacka",
    "RMF - Polska Prywatka", "RMF - Polski Rock", "RMF - Polskie Przeboje",
    "RMF - PRL", "RMF - Relaks", "RMF - Smooth Jazz", "RMF - Szanty",
    "RMF - W Pracy", "RMF - Cuba", "ESKA - Do Pracy", "ESKA - Impreska",
    "ESKA - Rap 20", "ESKA 2 - Kultowe Polskie", "ESKA Rock",
    "ESKA Rock - Klasyka Rocka", "ESKA Rock - Rock Ballads",
    "ESKA Rock - Teraz Polski Rock", "ESKA - Hity na Czasie",
    "Antyradio - Classic Rock", "Antyradio - Covers", "Antyradio - Greatest",
    "Antyradio - Made in Poland", "Antyradio - Makak", "Antyradio - Unplugged",
    "Antyradio - Ballads", "Antyradio - Hard", "Radio dla Ciebie - RDC",
    "Muzyczne Radio", "Polskie Radio PiK", "Radio e M", "Radio Rodzina",
    "Radio Victoria", "Bezpieczna Podróż", "Wasze Radio FM", "Radio Rekord",
    "Radio Mega", "Radio Disco", "Radio Nuta", "Weekend FM", "Radio Jard",
    "Radio Centrum", "Radio Fara", "Twoje Radio", "Radio Parada", "Radio 7",
    "Radio Park FM", "Radio Vanessa", "Radio Imperium", "Trendy Radio",
    "Radio 90", "Radio Fest", "Radio Leliwa", "Nasze Radio", "Radio Warta",
    "Radio Ram", "Radio Express", "Radio Kolor FM", "Folk Radio", "MC Radio",
    "Radio Gra", "Radio Fama", "Super FM", "Radio Norda FM",
    "Twoja Polska Stacja", "Pop Radio", "Radio Bon Ton", "Radio Alex",
    "Radio Jura", "Radio Strefa FM", "Radio Anioł Beskidów", "Radio Hit",
    "Radio Oko", "Radio RDN", "Radio Bayer FM", "Akademickie Radio LUZ",
    "Radio ONY", "Radio Italo4You", "Radio Afera", "Radio Jutrzenka",
]

FALLBACKS = {
    "rmf":       "rmf_fm",
    "zet":       "radio_zet",
    "antyradio": "antyradio",
    "eska":      "radio_eska",
}

MAX_STEM = 22

def normalize(name: str) -> str:
    nfkd = unicodedata.normalize("NFKD", name)
    ascii_str = nfkd.encode("ascii", errors="ignore").decode("ascii")
    result = ""
    for c in ascii_str:
        if c in (" ", "-"):
            if not result.endswith("_"):
                result += "_"
        elif c.isalnum() or c == "_":
            result += c
    return result[:MAX_STEM].lower()

# Znormalizowane nazwy stacji (bezpośrednie + przez fallback)
matched_files = set()
for s in stations:
    stem = normalize(s)
    matched_files.add(stem + ".raw")
    # fallback
    for prefix, fb_file in FALLBACKS.items():
        if stem.startswith(prefix):
            matched_files.add(fb_file + ".raw")

# Pliki .raw na SPIFFS (data/www/)
www_dir = os.path.join(os.path.dirname(__file__), "data", "www")
raw_files = sorted(f for f in os.listdir(www_dir) if f.endswith(".raw") and f != "logo_default.raw")

print("=== DOPASOWANE (zostawić) ===")
keep = sorted(f for f in raw_files if f in matched_files)
for f in keep:
    print(f"  {f}")

print(f"\n=== DO USUNIĘCIA ({len(raw_files)-len(keep)}/{len(raw_files)}) ===")
remove = sorted(f for f in raw_files if f not in matched_files)
for f in remove:
    print(f"  {f}")

print(f"\nZostawić: {len(keep)}, Usunąć: {len(remove)}")
input("\nNaciśnij Enter aby USUNĄĆ pliki (lub Ctrl+C aby anulować)...")

for f in remove:
    path = os.path.join(www_dir, f)
    os.remove(path)
    print(f"  Usunięto: {f}")

print("Gotowe.")
