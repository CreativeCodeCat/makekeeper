# MakeKeeper

<img src="makekeeper.svg" alt="MakeKeeper logo" width="112" align="right">

**A safer installer and manager for Makefile-based Linux applications.**

[Download](https://github.com/CreativeCodeCat/makekeeper/releases) ·
[Source code](https://github.com/CreativeCodeCat/makekeeper) ·
[Package catalog](packages.json) ·
[Report an issue](https://github.com/CreativeCodeCat/makekeeper/issues)

Plenty of Linux software, like the suckless tools and many small command-line utilities, ships with nothing but a `Makefile`. Installing it is easy: `make && sudo make install`. Keeping track of what landed where, updating it later, or removing it cleanly is not.

MakeKeeper handles that for you. It clones or finds the project, builds it, installs it, and keeps a registry of every package, so you can list, update and uninstall them later from the command line or a desktop app.

## Features

- **One command to install** from the package catalog, a Git URL, or a local directory
- **A registry of everything installed**, with each package's source, Git commit, install date and files
- **Clean uninstalls** that run the project's `make uninstall` and remove the files MakeKeeper tracked
- **Updates for Git packages**: pull, rebuild and reinstall in one step
- **A curated package catalog** you can sync and search
- **A desktop app** built with Qt 6 that follows your system theme
- **Root only where it's needed**: builds run as you, and only `make install` and `make uninstall` run as root, with one password prompt per command

## Install

### Prebuilt packages

Download the package for your distribution from the [releases page](https://github.com/CreativeCodeCat/makekeeper/releases), then install it from the folder you downloaded it to.

**Arch Linux** (`.pkg.tar.zst`)

```bash
sudo pacman -U ./makekeeper-*.pkg.tar.zst
```

**Debian and Ubuntu** (`.deb`)

```bash
sudo apt install ./makekeeper_*_amd64.deb
```

**Fedora** (`.rpm`)

```bash
sudo dnf install ./makekeeper-*.x86_64.rpm
```

**Any distribution** (`.AppImage`)

```bash
chmod +x MakeKeeper-*-x86_64.AppImage
./MakeKeeper-*-x86_64.AppImage
```

The AppImage still needs `git`, `make` and `pkexec` (polkit) installed on your system, because MakeKeeper uses them to build and install other software.

### From source

```bash
git clone https://github.com/CreativeCodeCat/makekeeper.git
cd makekeeper
make
sudo make install
```

On Arch Linux you can build a package instead with `makepkg -si`. The [project README](https://github.com/CreativeCodeCat/makekeeper#from-source) lists the build dependencies for each distribution.

## Quick start

```bash
makekeeper sync              # download the package catalog
makekeeper search terminal   # find something to install
makekeeper install st        # install it
makekeeper list              # see what's installed
makekeeper update            # update everything installed from Git
makekeeper uninstall st      # remove it again
```

Prefer a window? Run `makekeeper gui`, or open **MakeKeeper** from your application menu.

## Commands

| Command | What it does |
|---------|--------------|
| `install <package>` | Install a package from the catalog, a Git URL, or a local directory |
| `uninstall <name>` | Uninstall a package and delete MakeKeeper's copy of its source |
| `update [name]` | Update one package, or every package installed from Git |
| `list` | List installed packages |
| `info <name>` | Show details about an installed package |
| `search <query>` | Search the catalog's names and descriptions |
| `sync` | Download the latest package catalog |
| `fetch` | Print package statistics |
| `gui` | Open the desktop app |

`install` also takes `--name NAME` to register a package under a different name, and `--override` to reinstall one. `install` and `uninstall` show what they are about to do and ask first; add `-y` or `--yes` to skip the question, for example in scripts.

Run `makekeeper <command> --help` or `man makekeeper` for the details.

## How an install works

1. **Find the source.** Catalog names are looked up in the synced catalog, Git URLs are cloned into `~/.local/share/makekeeper/sources/<name>`, and directories are used where they are.
2. **Confirm.** MakeKeeper shows the package, its source and what will run as root, and asks before going ahead.
3. **Build** with `make`, as your user.
4. **Install** with `make install`, as root.
5. **Record** the package in the registry at `/var/lib/makekeeper/registry.json`.

## Security

A Makefile can run any command, and `make install` runs as root, so **only install projects you trust, and read their Makefile first**. Builds are not sandboxed.

MakeKeeper itself runs as your user. The only part that runs as root is a small helper, authorized through polkit, that can do a fixed set of things: run `make install`, `make uninstall`, `make clean` or `make` in a project, remove files the registry says MakeKeeper installed, and write MakeKeeper's own state.

## The package catalog

The catalog is [`packages.json`](packages.json), served from this site. `makekeeper sync` downloads it, and also looks up each package's latest version from its Git tags. Browse it with `makekeeper search`, or on the **Packages** page of the desktop app.

### Adding a package

Packages are added by pull request to the [`docs` branch](https://github.com/CreativeCodeCat/makekeeper/tree/docs), which this site is built from.

1. Add an entry to the `packages` list in `packages.json`:

   ```json
   {
     "name": "lf",
     "url": "https://github.com/gokcehan/lf.git",
     "description": "Terminal file manager",
     "category": "file-manager"
   }
   ```

   | Field | Value |
   |-------|-------|
   | `name` | A unique, lowercase name. It becomes the package's directory name, so use only letters, digits, `-`, `_` and `.` |
   | `url` | The repository's Git clone URL, preferably `https://` |
   | `description` | One short line saying what it is |
   | `category` | One of the existing categories, such as `system`, `terminal`, `utility`, `file-manager`, `window-manager`, `menu`, `media`, `audio` or `image` |

   Don't add `latest_version`; `sync` works it out from the repository's tags.

2. **Bump the top-level `"version"`**, for example from `1.0.4` to `1.0.5`. `sync` only downloads the catalog again when its version changes, so without this nobody gets your package.

3. Check the project works with MakeKeeper: its repository needs a `Makefile` at the top level where `make` builds it and `make install` installs it. A `make uninstall` target is strongly recommended, so uninstalling cleans up everything.

4. Open a pull request against the `docs` branch.

## License

MakeKeeper is free software under the [GNU General Public License v3.0](https://github.com/CreativeCodeCat/makekeeper/blob/main/LICENSE).
