#!/usr/bin/env bash
# Installs everything needed to build the Vita port, inside WSL (Ubuntu).
# Uses the softfp flavour of VitaSDK, which so-loader ports require.
#
# Run once from the Ubuntu terminal:
#   bash install_toolchain_wsl.sh
# then close and reopen the terminal so VITASDK is on your PATH.

set -euo pipefail

export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"

echo "== Installing Ubuntu build tools (asks for your Linux password)"
sudo apt-get update
sudo apt-get install -y build-essential git cmake python3 curl wget patch bzip2 xz-utils netcat-openbsd

echo "== Preparing $VITASDK"
sudo mkdir -p "$VITASDK"
sudo chown "$USER" "$VITASDK"

echo "== Installing VitaSDK (softfp) and its libraries"
rm -rf "$HOME/vdpm"
git clone https://github.com/vitasdk-softfp/vdpm "$HOME/vdpm"
cd "$HOME/vdpm"
./bootstrap-vitasdk.sh
./install-all.sh

if ! grep -q 'VITASDK=/usr/local/vitasdk' "$HOME/.bashrc"; then
  {
    echo ''
    echo '# PS Vita SDK (softfp)'
    echo 'export VITASDK=/usr/local/vitasdk'
    echo 'export PATH="$VITASDK/bin:$PATH"'
  } >> "$HOME/.bashrc"
fi

echo
echo "VitaSDK installed. Close and reopen this terminal, then run ./setup.sh"
