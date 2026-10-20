# Maintainer: makekeeper contributors
pkgname=makekeeper
pkgver=0.1.0
pkgrel=1
pkgdesc="A safer installer for Makefile-based Linux applications"
arch=('x86_64')
url="https://github.com/yourusername/makekeeper"
license=('GPL-3.0')
depends=()
makedepends=('rust' 'cargo')
options=('strip')

build() {
  cargo build --release
}

package() {
  install -Dm755 target/release/makekeeper "$pkgdir/usr/local/bin/makekeeper"
  install -Dm644 makekeeper.1 "$pkgdir/usr/local/share/man/man1/makekeeper.1"
}
