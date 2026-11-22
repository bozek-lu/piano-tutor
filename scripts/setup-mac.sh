#!/bin/bash
# Dev setup for piano-tutor on macOS (Apple Silicon or Intel). Safe to re-run.
# Installs: Homebrew packages (git, cmake, ninja, dfu-util, ccache, python 3.12, gitleaks),
# ESP-IDF v5.5.5 for the ESP32-P4 (Tab5), turns on the gitleaks pre-commit hook, and builds the firmware.
#   ./scripts/setup-mac.sh            full setup + build
#   ./scripts/setup-mac.sh --no-build skip the firmware build
set -euo pipefail

IDF_VERSION=v5.5.5
IDF_DIR="${IDF_DIR:-$HOME/esp/esp-idf-$IDF_VERSION}"
REPO="$(cd "$(dirname "$0")/.." && pwd)"
BUILD=1; [ "${1:-}" = "--no-build" ] && BUILD=0

step() { printf '\n==> %s\n' "$*"; }

step "Checking Xcode command line tools and Homebrew"
if ! xcode-select -p >/dev/null 2>&1; then
	echo "Run 'xcode-select --install', finish the installer, then run this script again."; exit 1
fi
if ! command -v brew >/dev/null 2>&1; then
	echo "Homebrew is missing. Install it from https://brew.sh, then run this script again."; exit 1
fi

step "Installing Homebrew packages"
brew install git cmake ninja dfu-util ccache python@3.12 gitleaks
# ESP-IDF's install.sh uses whatever python3 is first on PATH
export PATH="$(brew --prefix python@3.12)/libexec/bin:$PATH"

step "Enabling the gitleaks pre-commit hook"
git -C "$REPO" config core.hooksPath .githooks

step "Getting ESP-IDF $IDF_VERSION into $IDF_DIR"
if [ ! -d "$IDF_DIR/.git" ]; then
	mkdir -p "$(dirname "$IDF_DIR")"
	git clone -b "$IDF_VERSION" --recursive --depth 1 --shallow-submodules \
		https://github.com/espressif/esp-idf.git "$IDF_DIR"
else
	echo "already there"
fi

step "Installing ESP-IDF tools for esp32p4"
"$IDF_DIR/install.sh" esp32p4

if [ "$BUILD" = 1 ]; then
	step "Building the firmware"
	# shellcheck disable=SC1091
	. "$IDF_DIR/export.sh" >/dev/null
	cd "$REPO/firmware"
	idf.py set-target esp32p4
	idf.py build
fi

step "Done"
cat <<EOF
In a new terminal, activate ESP-IDF before building or flashing:
  . "$IDF_DIR/export.sh"
  cd "$REPO/firmware" && idf.py build
  idf.py -p /dev/cu.usbmodem* flash monitor     # Tab5 on its USB-C port
EOF
