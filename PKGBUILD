# Maintainer: makekeeper contributors
pkgname=makekeeper
pkgver=0.1.0
pkgrel=1
pkgdesc="A safer installer for Makefile-based Linux applications"
arch=('x86_64')
url="https://github.com/CreativeCodeCat/makekeeper"
license=('GPL-3.0')
depends=()
makedepends=('rust' 'cargo' 'git')
options=('strip')

source=("git+https://github.com/CreativeCodeCat/makekeeper.git")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname"
    cargo build --release
}

check() {
    cd "$srcdir/$pkgname"
    cargo test --release
}

package() {
    cd "$srcdir/$pkgname"

    install -Dm755 "target/release/makekeeper" \
        "$pkgdir/usr/bin/makekeeper"

    install -Dm644 "makekeeper.1" \
        "$pkgdir/usr/share/man/man1/makekeeper.1"
}