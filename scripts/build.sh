#!/usr/bin/env bash
# Builds the plugin into ./build (multichat.so). Run from anywhere.
# Reads optional credentials.env to bake in OAuth client IDs.
set -euo pipefail
cd "$(dirname "$0")/.."

creds=()
if [[ -f credentials.env ]]; then
	# shellcheck disable=SC1091
	source credentials.env
	creds=(
		-DTWITCH_CLIENT_ID="${TWITCH_CLIENT_ID:-}"
		-DYOUTUBE_CLIENT_ID="${YOUTUBE_CLIENT_ID:-}"
		-DYOUTUBE_CLIENT_SECRET="${YOUTUBE_CLIENT_SECRET:-}"
	)
fi

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr "${creds[@]}"
cmake --build build --parallel

echo
echo "Build complete: $(pwd)/build/multichat.so"
