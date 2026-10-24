#!/usr/bin/env bash

set -euo pipefail

script_folder="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_folder/.." && pwd)"
app_folder="$project_root/app"
app_data_folder="$app_folder/data"
mkdir -p "$app_folder" "$app_data_folder"

output_name="app"
no_run=false
reset_data=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-run)
            no_run=true
            shift
            ;;
        --reset-data)
            reset_data=true
            shift
            ;;
        --output)
            if [[ $# -lt 2 ]]; then
                printf 'Missing value for --output.\n' >&2
                exit 1
            fi
            output_name="$2"
            shift 2
            ;;
        -h|--help)
            printf 'Usage: %s [--no-run] [--reset-data] [--output NAME]\n' "$0"
            exit 0
            ;;
        *)
            printf 'Unknown option: %s\n' "$1" >&2
            printf 'Usage: %s [--no-run] [--reset-data] [--output NAME]\n' "$0" >&2
            exit 1
            ;;
    esac
done

if ! command -v clang++ >/dev/null 2>&1; then
    printf 'clang++ was not found. Install Xcode Command Line Tools first.\n' >&2
    exit 1
fi

include_folder=''
library_folder=''
raylib_flags=()
uses_pkg_config=false

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists raylib; then
    read -r -a raylib_flags <<< "$(pkg-config --cflags --libs raylib)"
    uses_pkg_config=true
elif command -v brew >/dev/null 2>&1 && [[ -f "$(brew --prefix raylib 2>/dev/null)/include/raylib.h" ]]; then
    raylib_prefix="$(brew --prefix raylib)"
    include_folder="$raylib_prefix/include"
    library_folder="$raylib_prefix/lib"
    raylib_flags=("-I$include_folder" "-L$library_folder" -lraylib)
elif [[ -f /opt/homebrew/include/raylib.h && -f /opt/homebrew/lib/libraylib.dylib ]]; then
    include_folder=/opt/homebrew/include
    library_folder=/opt/homebrew/lib
    raylib_flags=("-I$include_folder" "-L$library_folder" -lraylib)
elif [[ -f /usr/local/include/raylib.h && -f /usr/local/lib/libraylib.dylib ]]; then
    include_folder=/usr/local/include
    library_folder=/usr/local/lib
    raylib_flags=("-I$include_folder" "-L$library_folder" -lraylib)
else
    printf 'raylib was not found. Install it with: brew install raylib pkg-config\n' >&2
    exit 1
fi

source_list="$script_folder/sources.txt"
trap 'rm -f "$source_list"' EXIT
(
    cd "$script_folder"
    find . -type f -name '*.cpp' -not -path './Utils/raylib-*/*' | sed 's|^\./||' | LC_ALL=C sort
) > "$source_list"

sources=()
while IFS= read -r rel_file || [[ -n "$rel_file" ]]; do
    [[ -z "$rel_file" ]] && continue
    sources+=("$script_folder/$rel_file")
done < "$source_list"

if [[ ${#sources[@]} -eq 0 ]]; then
    printf 'No C++ source files were found.\n' >&2
    exit 1
fi

src_data_folder="$script_folder/data"
if ! command -v python3 >/dev/null 2>&1; then
    printf 'python3 was not found. Install Python 3 to generate the encrypted database files.\n' >&2
    exit 1
fi

SRC_DATA="$src_data_folder" APP_DATA="$app_data_folder" RESET_DATA="$reset_data" python3 - <<'PY'
import os
import secrets

src_dir = os.environ["SRC_DATA"]
dst_dir = os.environ["APP_DATA"]
reset = os.environ["RESET_DATA"] == "true"

def needs_update(src_path, dst_path):
    if reset or not os.path.exists(dst_path):
        return True
    return os.path.getmtime(src_path) > os.path.getmtime(dst_path)

account_src = os.path.join(src_dir, "accounts.txt")
account_dst = os.path.join(dst_dir, "accounts.dat")
if needs_update(account_src, account_dst):
    records = []
    with open(account_src, "r", encoding="utf-8") as file:
        for line_number, line in enumerate(file, 1):
            line = line.strip()
            if not line:
                continue
            fields = line.split("|")
            if len(fields) != 6:
                raise SystemExit(f"Invalid account record at line {line_number}")
            salt = secrets.token_hex(16)
            value = (salt + ":" + fields[5]).encode("utf-8")
            hash_value = 14695981039346656037
            for byte in value:
                hash_value = ((hash_value ^ byte) * 1099511628211) & ((1 << 64) - 1)
            records.append("|".join(fields[:5] + [salt, f"{hash_value:016x}"]) + "\n")
    encoded = bytes(byte ^ 0x5A for byte in "".join(records).encode("utf-8"))
    with open(account_dst, "wb") as file:
        file.write(encoded)
    print(f"Generated encrypted account database: {account_dst}")

for name in ["branches", "menu", "ingredients", "inventory", "orders", "schedules", "attendance"]:
    src_path = os.path.join(src_dir, f"{name}.txt")
    dst_path = os.path.join(dst_dir, f"{name}.dat")
    if needs_update(src_path, dst_path):
        with open(src_path, "rb") as file:
            raw = file.read()
        with open(dst_path, "wb") as file:
            file.write(bytes(b ^ 0x5A for b in raw))
        print(f"Generated encrypted data file: {dst_path}")
PY

app_path="$app_folder/$output_name"
compile_args=(-std=c++20 -O2 -Wall -Wextra "${raylib_flags[@]}" "${sources[@]}" -o "$app_path")

if [[ "$uses_pkg_config" == false ]]; then
    compile_args+=(-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo)
fi

printf 'Building %s...\n' "$app_path"
clang++ "${compile_args[@]}"

if [[ "$no_run" == false ]]; then
    printf 'Running: %s\n' "$app_path"
    (cd "$app_folder" && "$app_path")
fi
