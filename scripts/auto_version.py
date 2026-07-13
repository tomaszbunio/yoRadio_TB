from pathlib import Path
import re

Import("env")

try:
    from SCons.Script import COMMAND_LINE_TARGETS
except Exception:
    COMMAND_LINE_TARGETS = []


def is_upload_target() -> bool:
    return any(str(target).lower() == "upload" for target in COMMAND_LINE_TARGETS)


def read_text(path: Path) -> str:
    for encoding in ("utf-8-sig", "utf-8", "cp1250"):
        try:
            return path.read_text(encoding=encoding)
        except UnicodeDecodeError:
            continue
    return path.read_text()


def detect_newline(text: str) -> str:
    return "\r\n" if "\r\n" in text else "\n"


def bump_firmware_version() -> None:
    if not is_upload_target():
        return

    project_dir = Path(env.subst("$PROJECT_DIR"))
    myoptions = project_dir / "myoptions.h"
    text = read_text(myoptions)
    newline = detect_newline(text)

    pattern = re.compile(
        r'(#define\s+FIRMWARE_VERSION\s+")'
        r'(?P<base>[^"\r\n]*?-build\.)'
        r'(?P<num>\d+)'
        r'(")'
    )

    match = pattern.search(text)
    if not match:
        raise RuntimeError(
            "auto_version: missing '#define FIRMWARE_VERSION \"...-build.N\"' in myoptions.h"
        )

    old_num = match.group("num")
    new_num = str(int(old_num) + 1).zfill(len(old_num))
    old_version = f'{match.group("base")}{old_num}'
    new_version = f'{match.group("base")}{new_num}'

    updated = pattern.sub(
        rf'\g<1>{match.group("base")}{new_num}\4',
        text,
        count=1,
    )

    if newline == "\r\n":
        updated = updated.replace("\r\n", "\n").replace("\n", "\r\n")

    myoptions.write_text(updated, encoding="utf-8")
    print(f"auto_version: FIRMWARE_VERSION {old_version} -> {new_version}")


bump_firmware_version()
