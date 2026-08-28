#!/usr/bin/env bash

set -euo pipefail

script_folder="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
output_name="app"
no_run=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-run)
            no_run=true
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
            printf 'Usage: %s [--no-run] [--output NAME]\n' "$0"
            exit 0
            ;;
        *)
            printf 'Unknown option: %s\n' "$1" >&2
            printf 'Usage: %s [--no-run] [--output NAME]\n' "$0" >&2
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

sources=()
while IFS= read -r -d '' source_file; do
    sources+=("$source_file")
done < <(find "$script_folder" -type f -name '*.cpp' \
    -not -path "$script_folder/Utils/raylib-*/*" -print0)

if [[ ${#sources[@]} -eq 0 ]]; then
    printf 'No C++ source files were found.\n' >&2
    exit 1
fi

app_path="$script_folder/$output_name"
compile_args=(-std=c++20 -Wall -Wextra "${raylib_flags[@]}" "${sources[@]}" -o "$app_path")

# raylib's pkg-config file includes these platform frameworks when available.
if [[ "$uses_pkg_config" == false ]]; then
    compile_args+=(-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo)
fi

printf 'Building %s...\n' "$app_path"
clang++ "${compile_args[@]}"

if [[ "$no_run" == false ]]; then
    printf 'Running: %s\n' "$app_path"
    (cd "$script_folder" && "$app_path")
fi
