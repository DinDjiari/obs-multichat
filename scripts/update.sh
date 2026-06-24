#!/usr/bin/env bash
# Clean rebuild and (re)install the plugin via pacman.
# Run as your normal user (makepkg calls sudo pacman itself); do NOT run as root.
set -euo pipefail
cd "$(dirname "$0")/.."

echo "==> Removing previous build output..."
# Removes makepkg's build/output dirs. NOT the src/ source tree.
rm -rf build pkg ./*.pkg.tar.*

echo "==> Building and installing via pacman..."
# -s install build deps, -i install with pacman (-U), -f force rebuild
# even if the version is unchanged.
makepkg -sif

# Seed the runtime credentials file only if it does NOT exist yet. An existing
# one (with your real keys) is never overwritten by a build.
if [[ -f credentials.env && ! -f "$HOME/.config/obs-multichat/credentials.env" ]]; then
	mkdir -p "$HOME/.config/obs-multichat"
	cp credentials.env "$HOME/.config/obs-multichat/credentials.env"
	chmod 600 "$HOME/.config/obs-multichat/credentials.env"
	echo "Seeded ~/.config/obs-multichat/credentials.env (edit it with your keys)."
else
	echo "Keeping existing ~/.config/obs-multichat/credentials.env (not overwritten)."
fi

echo
echo "Done. Restart OBS to load the updated plugin."
