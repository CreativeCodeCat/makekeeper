# Maintainer: makekeeper contributors
pkgname=makekeeper
pkgver=0.1.0
pkgrel=1
pkgdesc="A safer installer for Makefile-based Linux applications"
arch=('x86_64')
url="https://github.com/CreativeCodeCat/makekeeper"
license=('GPL-3.0')
depends=('qt6-base' 'libgit2' 'curl' 'git' 'make' 'polkit')
makedepends=('cmake' 'git' 'nlohmann-json')
options=('strip' '!debug')

source=("git+https://github.com/CreativeCodeCat/makekeeper.git")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname"
    mkdir -p build
    cd build
    cmake ..
    make
}

check() {
    cd "$srcdir/$pkgname"
    cd build
    ctest --output-on-failure
}

package() {
    cd "$srcdir/$pkgname"

    install -Dm755 "build/makekeeper" \
        "$pkgdir/usr/bin/makekeeper"

    install -Dm644 "makekeeper.1" \
        "$pkgdir/usr/share/man/man1/makekeeper.1"

    install -Dm644 "makekeeper.fish" \
        "$pkgdir/usr/share/fish/vendor_completions.d/makekeeper.fish"

    install -Dm644 "makekeeper.desktop" \
        "$pkgdir/usr/share/applications/makekeeper.desktop"

    install -Dm644 "makekeeper-128.png" \
        "$pkgdir/usr/share/icons/hicolor/128x128/apps/makekeeper-128.png"

    install -Dm644 "makekeeper.svg" \
        "$pkgdir/usr/share/icons/hicolor/scalable/apps/makekeeper.svg"
}