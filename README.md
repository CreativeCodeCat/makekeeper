# makekeeper

A safer installer for Makefile-based Linux applications.

makekeeper provides a controlled way to install applications that use GNU Make build systems. It tracks all installed files and managed symlinks, enabling safe uninstallation and preventing conflicts with existing installations.

## Features

- **Tracked installations**: Records all files and symlinks created during installation
- **Safe uninstallation**: Removes only unchanged files, preserving user modifications
- **Conflict detection**: Prevents overwriting existing files or symlinks
- **Preview before install**: Inspect Makefiles and preview installation commands
- **Executable symlinks**: Automatically links executables from `bin/` directories to `/usr/local/bin`

## Installation

### From source

```bash
cargo build --release
sudo make install
```

## Usage

### Install a project

```bash
sudo makekeeper install /path/to/project
```

This will:
1. Build the project using `make`
2. Stage the installation to a temporary directory
3. Check for conflicts with existing files
4. Ask for confirmation before proceeding
5. Install to `/opt/makekeeper/apps/<name>`
6. Create symlinks in `/usr/local/bin` for executables

You can specify a custom name:

```bash
sudo makekeeper install /path/to/project --name myapp
```

Or skip confirmation with `--yes`:

```bash
sudo makekeeper install /path/to/project --yes
```

### Inspect a project

Preview what will be installed without actually installing:

```bash
makekeeper inspect /path/to/project
```

This shows:
- Possible Make targets
- A dry-run preview of the `make install` command

### List installed applications

```bash
sudo makekeeper list
```

### View installation details

```bash
sudo makekeeper info <name>
```

Shows the source, prefix, installed files, and managed symlinks.

### Uninstall an application

```bash
sudo makekeeper uninstall <name>
```

This removes:
- Unchanged tracked files
- Unchanged managed symlinks
- Empty directories in the installation prefix

Files that have been modified are preserved for manual review.

Skip confirmation with `--yes`:

```bash
sudo makekeeper uninstall <name> --yes
```

## Installation Directory Structure

Applications are installed to:

```
/opt/makekeeper/apps/<name>/
```

Executable symlinks are created in:

```
/usr/local/bin/
```

Installation records are stored in:

```
/var/lib/makekeeper/<name>.json
```

## Security Considerations

- makekeeper requires root privileges for system-wide installation
- Makefiles can execute arbitrary code—only install projects you trust
- Always inspect a project with `makekeeper inspect` before installing
- The `make install` step is not sandboxed

## Requirements

- Linux system
- GNU Make
- Rust toolchain (for building makekeeper itself)
- Root/sudo access for installation and management

## License

This project is provided as-is for safer Makefile-based application management.
