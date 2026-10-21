# Fish completion for makekeeper

complete -c makekeeper -f

# Main command
complete -c makekeeper -n "__fish_use_subcommand" -a inspect -d "Inspect a project's Makefile"
complete -c makekeeper -n "__fish_use_subcommand" -a install -d "Build and install a project"
complete -c makekeeper -n "__fish_use_subcommand" -a git -d "Clone a git repository and install it"
complete -c makekeeper -n "__fish_use_subcommand" -a list -d "List registered applications"
complete -c makekeeper -n "__fish_use_subcommand" -a info -d "Display an installation record"
complete -c makekeeper -n "__fish_use_subcommand" -a uninstall -d "Remove tracked files and symlinks"
complete -c makekeeper -n "__fish_use_subcommand" -a update -d "Update Git-based installations"
complete -c makekeeper -n "__fish_use_subcommand" -a fetch -d "Show installation statistics"
complete -c makekeeper -n "__fish_use_subcommand" -a sync -d "Sync package catalog"
complete -c makekeeper -n "__fish_use_subcommand" -a search -d "Search package catalog"

# inspect command
complete -c makekeeper -n "__fish_seen_subcommand_from inspect" -r -d "Project path"

# install command
complete -c makekeeper -n "__fish_seen_subcommand_from install" -l dir -r -d "Directory to install from"
complete -c makekeeper -n "__fish_seen_subcommand_from install" -l name -r -d "Custom name for installation"
complete -c makekeeper -n "__fish_seen_subcommand_from install" -l yes -d "Skip confirmation prompt"
complete -c makekeeper -n "__fish_seen_subcommand_from install" -l override -d "Override existing installation"
complete -c makekeeper -n "__fish_seen_subcommand_from install" -l verbose -d "Show detailed output"

# git command
complete -c makekeeper -n "__fish_seen_subcommand_from git" -r -d "Git repository URL"
complete -c makekeeper -n "__fish_seen_subcommand_from git" -l name -r -d "Custom name for installation"
complete -c makekeeper -n "__fish_seen_subcommand_from git" -l yes -d "Skip confirmation prompt"
complete -c makekeeper -n "__fish_seen_subcommand_from git" -l override -d "Override existing installation"
complete -c makekeeper -n "__fish_seen_subcommand_from git" -l verbose -d "Show detailed output"

# info command - complete with installed application names
complete -c makekeeper -n "__fish_seen_subcommand_from info" -a "(makekeeper list 2>/dev/null | string match -r '^[^ ]+' | string trim)" -d "Installed application"

# uninstall command
complete -c makekeeper -n "__fish_seen_subcommand_from uninstall" -a "(makekeeper list 2>/dev/null | string match -r '^[^ ]+' | string trim)" -d "Installed application"
complete -c makekeeper -n "__fish_seen_subcommand_from uninstall" -l yes -d "Skip confirmation prompt"
complete -c makekeeper -n "__fish_seen_subcommand_from uninstall" -l verbose -d "Show detailed output"

# update command
complete -c makekeeper -n "__fish_seen_subcommand_from update" -l yes -d "Skip confirmation prompt"
complete -c makekeeper -n "__fish_seen_subcommand_from update" -l verbose -d "Show detailed output"

# search command
complete -c makekeeper -n "__fish_seen_subcommand_from search" -r -d "Search query"

# Global options
complete -c makekeeper -l help -d "Print help information"
complete -c makekeeper -l version -d "Print version information"
