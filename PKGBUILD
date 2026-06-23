# Maintainer: Multichat
pkgname=obs-multichat
pkgver=1.0.0
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
	cmake -B "$startdir/build" -S "$startdir" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/usr
	cmake --build "$startdir/build" --parallel
}

package() {
	DESTDIR="$pkgdir" cmake --install "$startdir/build"
}
