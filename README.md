<p align="center">
  <img src="makekeeper.svg" alt="MakeKeeper logo" width="128">
</p>

<h1 align="center">MakeKeeper</h1>

<p align="center">
  A safer installer and manager for Makefile-based Linux applications.
</p>

---

Plenty of Linux software (suckless tools, small CLI utilities, dotfile-style projects) ships with nothing but a `Makefile`. Installing it is easy — `make && sudo make install` — but keeping track of what landed where, updating it later, or removing it cleanly is not.

MakeKeeper wraps that workflow. It clones or locates the project, builds and installs it, and keeps a registry of every package so you can list, inspect, update and uninstall them later from the command line or a Qt GUI.

## Features

- **One command to install**: install from the package catalog, a Git URL, or a local directory
- **Package registry**: records the source, Git commit, install date and tracked files for every package
- **Clean uninstall**: runs the project's `make uninstall`, then removes the tracked files and symlinks
- **Git-aware updates**: pulls, rebuilds and reinstalls packages installed from a Git repository
- **Package catalog**: sync and search a curated list of Makefile-based applications
- **Desktop GUI**: browse, install and manage packages from a Qt 6 interface
- **One password prompt per operation**: a small privileged helper does the root-only steps, authorized once through polkit

## Installation

### Prebuilt packages

The [releases](https://github.com/CreativeCodeCat/makekeeper/releases) page provides `.pkg.tar.zst` (Arch), `.deb` (Debian/Ubuntu) and `.rpm` (Fedora/openSUSE) packages.

### Arch Linux (PKGBUILD)

```bash
git clone https://github.com/CreativeCodeCat/makekeeper.git
cd makekeeper
makepkg -si
```

### From source

Install the dependencies first:

| Distribution  | Command |
|---------------|---------|
| Arch          | `sudo pacman -S cmake git qt6-base libgit2 curl nlohmann-json polkit` |
| Debian/Ubuntu | `sudo apt install cmake g++ git qt6-base-dev libgit2-dev libcurl4-openssl-dev nlohmann-json3-dev policykit-1` |
| Fedora        | `sudo dnf install cmake gcc-c++ git qt6-qtbase-devel libgit2-devel libcurl-devel json-devel polkit` |

Then build and install:

```bash
git clone https://github.com/CreativeCodeCat/makekeeper.git
cd makekeeper
make
sudo make install
```

This installs:

| File | Location |
|------|----------|
| Binary | `/usr/local/bin/makekeeper` |
| Privileged helper | `/usr/local/lib/makekeeper/makekeeper-helper` |
| polkit policy | `/usr/share/polkit-1/actions/io.github.creativecodecat.makekeeper.policy` |
| Man page | `/usr/local/share/man/man1/makekeeper.1` |
| Fish completions | `/usr/share/fish/vendor_completions.d/makekeeper.fish` |
| Desktop entry | `/usr/local/share/applications/makekeeper.desktop` |
| Icons | `/usr/local/share/icons/hicolor/{128x128,scalable}/apps/` |

Override the location with `PREFIX`, for example `sudo make install PREFIX=/usr`. Remove it with `sudo make uninstall`.

## Quick start

```bash
makekeeper sync              # download the package catalog
makekeeper search terminal   # find something to install
makekeeper install st        # install it
makekeeper list              # see what's installed
```

## Usage

```
makekeeper <COMMAND> [OPTIONS]
```

| Command | Description |
|---------|-------------|
| `install <NAME\|URL\|DIR>` | Install a package from the catalog, a Git URL, or a local directory |
| `uninstall <NAME>` | Uninstall a package |
| `update [NAME]` | Update one package, or check all Git-based packages |
| `list` | List installed packages |
| `info <NAME>` | Show details about an installed package |
| `search <QUERY>` | Search the package catalog |
| `sync` | Download the latest package catalog |
| `fetch` | Print package statistics (`installed/git (makekeeper)`) |
| `gui` | Launch the graphical interface |

Run `makekeeper <COMMAND> --help` for help on a single command, and `man makekeeper` for the full manual.

### Installing packages

`install` takes a catalog package name, a Git URL or a path to a directory:

```bash
makekeeper install neofetch                                 # from the catalog
makekeeper install https://github.com/dylanaraps/pfetch.git # from a Git URL
makekeeper install ~/src/myproject                          # from a local directory
```

| Option | Description |
|--------|-------------|
| `--name NAME` | Register the package under a custom name |
| `--override` | Reinstall a package that is already installed |
| `-y`, `--yes` | Don't ask for confirmation |

The install steps are:

1. **Resolve the source.** Catalog names are looked up in the synced catalog, URLs are cloned as your user into `~/.local/share/makekeeper/sources/<name>`, and directories are used in place.
2. **Confirm.** MakeKeeper shows the package, its source and what will run as root, and asks before going ahead.
3. **Build** with `make`, as your user (skipped if a `build/`, `target/` or `bin/` directory already exists).
4. **Install** with `make install`, as root.
5. **Record** the package, its source and Git commit in the registry.

Without a terminal to ask on, such as in a script, `install` and `uninstall` refuse to continue unless you pass `--yes`.

### Updating packages

```bash
makekeeper update dwm   # pull, clean, rebuild and reinstall dwm if the remote has changes
makekeeper update       # fetch updates for every Git-based package
```

Packages installed from a local directory with no Git remote are skipped.

Older versions kept root-owned clones in `/var/lib/makekeeper/cache/`. The next `update` of such a package clones it again into your sources directory and deletes the old copy.

### Uninstalling packages

```bash
makekeeper uninstall neofetch
```

This runs `make uninstall` in the package's source directory (if it still exists), removes the files and symlinks tracked in the registry, deletes MakeKeeper's clone of the package, and deletes the registry entry. It lists what will be removed and asks first; pass `-y` or `--yes` to skip the question.

### The GUI

```bash
makekeeper gui
```

Or launch **MakeKeeper** from your application menu. The GUI has views for an overview, browsing the catalog, pending updates, the installed-package registry and settings.

## File locations

| Path | Purpose |
|------|---------|
| `/var/lib/makekeeper/registry.json` | Registry of installed packages |
| `/var/lib/makekeeper/cache/packages.json` | Synced package catalog |
| `~/.local/share/makekeeper/sources/<name>/` | Your clones of Git-based packages (deleted on uninstall) |

The catalog is fetched from <https://creativecodecat.github.io/makekeeper/packages.json>. Its source is [`packages.json` on the `docs` branch](https://github.com/CreativeCodeCat/makekeeper/blob/docs/packages.json), which the project's GitHub Pages site is built from.

## Security

> [!WARNING]
> A Makefile can run any command, and MakeKeeper runs `make install` and `make uninstall` as root. Only install projects you trust, and read their Makefile first.

- MakeKeeper itself runs as your user. Only `makekeeper-helper` runs as root, through `pkexec`, so a polkit authentication agent must be running.
- The helper can only do a fixed set of things: run `make`, `make install`, `make uninstall` or `make clean` in a project, remove files the registry says MakeKeeper installed, and write MakeKeeper's own state in `/var/lib/makekeeper`.
- polkit remembers your authorization for a few minutes, so one install or uninstall asks for your password once. The AppImage has no polkit policy installed, so it may ask once per step.
- Git operations run as your user, so your SSH keys and Git credentials work as usual.
- Builds are **not** sandboxed.

## Adding a package to the catalog

The catalog is a JSON file on the `docs` branch. To suggest a package, open a pull request against the `docs` branch that adds an entry to [`packages.json`](https://github.com/CreativeCodeCat/makekeeper/blob/docs/packages.json):

```json
{
  "name": "lf",
  "url": "https://github.com/gokcehan/lf.git",
  "description": "Terminal file manager",
  "category": "file-manager"
}
```

In the same pull request, bump the catalog's top-level `"version"` (for example from `1.0.4` to `1.0.5`). `makekeeper sync` only downloads the catalog again when its version changes.

The repository must have a `Makefile` at its root with `install` (and ideally `uninstall`) targets. The [project site](https://creativecodecat.github.io/makekeeper/#adding-a-package) has the full guide, including what each field means.

## Development

```bash
make            # configure with CMake and build into ./build
make clean      # remove the build directory
```

The build is configured with CMake (C++17, Qt 6). It exports `compile_commands.json` for editor tooling.

```
src/
├── main.cpp         # Entry point and command registration
├── cli.cpp          # Command dispatch, help and version output
├── config.h         # Paths, catalog URL and version
├── registry.cpp     # Installed-package registry (JSON)
├── utils.cpp        # Process execution and running makekeeper-helper
├── commands/        # One file per CLI command
├── helper/          # makekeeper-helper, the only code that runs as root
└── gui/             # Qt main window and views
    └── views/       # overview, packages, updates, registry, settings
```

## License

MakeKeeper is licensed under the [GNU General Public License v3.0](LICENSE).
