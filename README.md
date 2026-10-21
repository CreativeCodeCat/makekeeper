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
- **Git repository support**: Install directly from git repositories with automatic caching
- **Update detection**: Check for and update Git-based installations when remote changes
- **Package catalog**: Browse and install from a curated list of Makefile-based applications
- **Modular architecture**: Clean codebase with organized modules for maintainability

## Installation

### From source

```bash
cargo build --release
make install
```

This installs:
- The binary to `/usr/local/bin/makekeeper`
- The man page to `/usr/local/share/man/man1/makekeeper.1`
- Fish shell completions to `/usr/share/fish/vendor_completions.d/makekeeper.fish`

Note: makekeeper automatically uses pkexec for operations requiring root privileges.

## Usage

### Install from a package catalog

```bash
makekeeper install <package-name>
```

Install from the curated package catalog:

```bash
makekeeper install neofetch
```

Install with a custom name:

```bash
makekeeper install neofetch --name myneofetch
```

### Install from a directory

```bash
makekeeper install --dir /path/to/project
```

Install from a specific directory:

```bash
makekeeper install --dir /path/to/project --name myapp
```

Install from current directory:

```bash
makekeeper install --name myapp
```

This will:
1. Build the project using `make`
2. Stage the installation to a temporary directory
3. Check for conflicts with existing files (warns before overwriting)
4. Ask for confirmation before proceeding
5. Install to `/opt/makekeeper/apps/<name>`
6. Create symlinks in `/usr/local/bin` for executables
7. Create symlinks in `/usr/share/man` for man pages

Skip confirmation with `--yes`:

```bash
makekeeper install --dir /path/to/project --yes
```

Override an existing installation:

```bash
makekeeper install --dir /path/to/project --override
```

Show detailed installation output with `--verbose`:

```bash
makekeeper install --dir /path/to/project --verbose
```

### Install from a git repository

```bash
makekeeper git <git-url>
```

This will:
1. Clone the repository to a cache directory
2. Build and install the project using the same process as `install`
3. Keep the repository cached for future updates

You can specify a custom name:

```bash
makekeeper git <git-url> --name myapp
```

Or skip confirmation with `--yes`:

```bash
makekeeper git <git-url> --yes
```

Override an existing installation:

```bash
makekeeper git <git-url> --override
```

Show detailed installation output with `--verbose`:

```bash
makekeeper git <git-url> --verbose
```

### Sync package catalog

```bash
makekeeper sync
```

Download the latest package catalog from the remote server. This is required before using package-based installation or search.

### Search packages

```bash
makekeeper search <query>
```

Search the package catalog for applications matching your query. Searches across package names, descriptions, categories, and URLs.

### Update Git-based installations

```bash
makekeeper update
```

Check for updates to all Git-based installations and rebuild any that have changed on the remote repository.

### Fetch statistics

```bash
makekeeper fetch
```

Show the count of installed packages and Git-tracked packages.

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
makekeeper list
```

### View installation details

```bash
makekeeper info <name>
```

Shows the source, prefix, installed files, and managed symlinks.

### Uninstall an application

```bash
makekeeper uninstall <name>
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

Show detailed removal output with `--verbose`:

```bash
sudo makekeeper uninstall <name> --verbose
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

- makekeeper automatically uses pkexec for operations requiring root privileges
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
    ├── git.rs        # Install from git repository
    ├── update.rs     # Update Git-based installations
    ├── list.rs       # List installed applications
    ├── info.rs       # Show application details
    ├── uninstall.rs  # Uninstall an application
    ├── sync.rs       # Sync package catalog
    ├── search.rs     # Search package catalog
    └── fetch.rs      # Show installation statistics
```

## License

This project is provided as-is for safer Makefile-based application management.
