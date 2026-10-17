#!/usr/bin/env bash
# Monster Hunter Stories -> PS Vita: create the build tree.
#
# Clones soloader-boilerplate (MIT, v-atamanenko) into ./build-tree, copies the
# MHS-specific files from ./overlay on top, and adjusts CMakeLists.txt and
# dynlib.c. Re-run it any time; it starts from a clean clone each time.
#
# Run from Linux, macOS or WSL with VitaSDK installed and $VITASDK set.

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
TREE="$HERE/build-tree"
BOILERPLATE_URL="https://github.com/v-atamanenko/soloader-boilerplate"

# Some boilerplate submodules use SSH URLs (git@github.com:...). Fetch them over
# HTTPS instead so no SSH key is needed (CI containers don't even have ssh).
git_https() { git -c url."https://github.com/".insteadOf="git@github.com:" "$@"; }

rm -rf "$TREE"
git_https clone --recurse-submodules "$BOILERPLATE_URL" "$TREE"

# Optional: build against a specific boilerplate commit/branch (BOILERPLATE_REF=...)
if [ -n "${BOILERPLATE_REF:-}" ]; then
  git -C "$TREE" checkout "$BOILERPLATE_REF"
  git_https -C "$TREE" submodule update --init --recursive
fi
echo "Using soloader-boilerplate $(git -C "$TREE" rev-parse --short HEAD)"

# 1. Overlay our sources
cp -r "$HERE/overlay/." "$TREE/"

cd "$TREE"

# 2. CMakeLists.txt: app identity, data path, extra sources
sed -i \
  -e 's|set(VITA_APP_NAME ".*")|set(VITA_APP_NAME "Monster Hunter Stories")|' \
  -e 's|set(VITA_TITLEID ".*")|set(VITA_TITLEID "MHST00001")|' \
  -e 's|set(VITA_VPKNAME ".*")|set(VITA_VPKNAME "mhs_vita")|' \
  -e 's|set(DATA_PATH ".*")|set(DATA_PATH "ux0:data/mhs/")|' \
  -e 's|set(SO_PATH ".*")|set(SO_PATH "${DATA_PATH}libMHR.so")|' \
  -e 's|    source/patch.c|    source/patch.c\n    source/mhs/mtfp_event.c\n    source/mhs/dynlib_extra.c|' \
  CMakeLists.txt

grep -q 'source/mhs/mtfp_event.c' CMakeLists.txt || { echo "setup: failed to add sources to CMakeLists.txt" >&2; exit 1; }
grep -q 'MHST00001' CMakeLists.txt || { echo "setup: failed to set title id in CMakeLists.txt" >&2; exit 1; }

# 3. dynlib.c: our header up top, our entries first in the table
sed -i '0,/^#include/s||#include "mhs/dynlib_extra.h"\n#include|' source/dynlib.c
sed -i '/so_default_dynlib default_dynlib\[\] = {/a #include "mhs/dynlib_extra.inc"' source/dynlib.c

grep -q 'mhs/dynlib_extra.inc' source/dynlib.c || { echo "setup: failed to patch dynlib.c" >&2; exit 1; }

# 4. glutil.c: no MSAA. 4x MSAA costs memory vitaGL needs and a lot of GPU
#    bandwidth; the game is far too heavy for it on the Vita.
sed -i 's/SCE_GXM_MULTISAMPLE_4X/SCE_GXM_MULTISAMPLE_NONE/' source/utils/glutil.c
grep -q 'SCE_GXM_MULTISAMPLE_NONE' source/utils/glutil.c || { echo "setup: failed to disable MSAA" >&2; exit 1; }

echo
echo "Build tree ready in $TREE"
echo "Next:"
echo "  cmake -S build-tree -B build-tree/build -DCMAKE_BUILD_TYPE=Debug -DDUMP_COMPILED_SHADERS=ON"
echo "  cmake --build build-tree/build -j"
