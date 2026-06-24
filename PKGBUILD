# Maintainer: Multichat
pkgname=obs-multichat
pkgver=3.3.7
pkgrel=1
pkgdesc="OBS Studio dock combining Twitch and YouTube live chat and alerts"
arch=('x86_64')
url="https://example.com/obs-multichat"
license=('GPL2')
depends=('obs-studio' 'qt6-base' 'qt6-websockets' 'libsecret')
makedepends=('cmake' 'gcc' 'pkgconf')

# This PKGBUILD builds from the surrounding project directory.
# Run `makepkg -si` from the repository root (where this file lives).

build() {
	local creds=()
	if [[ -f "$startdir/credentials.env" ]]; then
		# shellcheck disable=SC1091
		source "$startdir/credentials.env"
		creds=(
			-DTWITCH_CLIENT_ID="${TWITCH_CLIENT_ID:-}"
			-DYOUTUBE_CLIENT_ID="${YOUTUBE_CLIENT_ID:-}"
			-DYOUTUBE_CLIENT_SECRET="${YOUTUBE_CLIENT_SECRET:-}"
		)
	fi
	cmake -B "$startdir/build" -S "$startdir" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/usr \
		"${creds[@]}"
	cmake --build "$startdir/build" --parallel
}

package() {
	DESTDIR="$pkgdir" cmake --install "$startdir/build"
}
