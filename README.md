# makekeeper

A safer installer for Makefile-based Linux applications.

makekeeper provides a controlled way to install applications that use GNU Make build systems. It tracks all installed files and managed symlinks, enabling safe uninstallation and preventing conflicts with existing installations.

## Features

- **Tracked installations**: Records all files and symlinks created during installation
- **Safe uninstallation**: Removes only unchanged files, preserving user modifications
- **Conflict warnings**: Warns before overwriting existing files or symlinks
- **Preview before install**: Inspect Makefiles and preview installation commands
- **Executable symlinks**: Automatically links executables from `bin/` directories to `/usr/local/bin`
- **Man page support**: Automatically symlinks man pages to `/usr/share/man`
- **Modular architecture**: Clean codebase with organized modules for maintainability

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
3. Check for conflicts with existing files (warns before overwriting)
4. Ask for confirmation before proceeding
5. Install to `/opt/makekeeper/apps/<name>`
6. Create symlinks in `/usr/local/bin` for executables
7. Create symlinks in `/usr/share/man` for man pages

You can specify a custom name:

```bash
sudo makekeeper install /path/to/project --name myapp
```

Or skip confirmation with `--yes`:

```bash
sudo makekeeper install /path/to/project --yes
```

### Install from a git repository

```bash
sudo makekeeper git <git-url>
```

This will:
1. Clone the repository to a temporary directory
2. Build and install the project using the same process as `install`
3. Clean up the cloned directory after installation

Note: This command requires root privileges (sudo).

You can specify a custom name:

```bash
sudo makekeeper git <git-url> --name myapp
```

Or skip confirmation with `--yes`:

```bash
sudo makekeeper git <git-url> --yes
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

## Development

Build the project:

```bash
cargo build --release
```

Run tests:

```bash
cargo test
```

Check code without building:

```bash
cargo check
```

The codebase is organized into modules for clarity and maintainability. Each command has its own module in `src/commands/`, and shared functionality is separated into utility modules.

## Project Structure

```
src/
├── main.rs           # Entry point and CLI dispatch
├── cli.rs            # CLI argument definitions
├── config.rs         # Configuration constants
├── models.rs         # Data structures (Record, InstalledFile, ManagedLink)
├── utils.rs          # Utility functions
├── registry.rs       # Registry operations
└── commands/         # Command implementations
    ├── inspect.rs    # Inspect project Makefile
    ├── install.rs    # Install a project
    ├── list.rs       # List installed applications
    ├── info.rs       # Show application details
    └── uninstall.rs  # Uninstall an application
```

## License

This project is provided as-is for safer Makefile-based application management.
