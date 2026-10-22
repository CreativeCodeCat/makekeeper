# Fish completion for makekeeper

complete -c makekeeper -f

# Installed packages from 'makekeeper list', described by their URL
function __makekeeper_installed_packages
    set -l name
    for line in (makekeeper list 2>/dev/null)
        if set -l match (string replace -rf '^  (\S+)$' '$1' -- $line)
            set name $match
        else if test -n "$name"
            # The line after each name is its URL
            set -l url (string replace -r '^\s*URL:\s*' '' -- $line)
            test "$url" = N/A; and set url "Installed from a local directory"
            printf '%s\t%s\n' $name $url
            set name
        end
    end
end

# Catalog packages from 'makekeeper search' (an empty query matches every package), with their descriptions
function __makekeeper_catalog_packages
    set -l name
    for line in (makekeeper search '' 2>/dev/null)
        if set -l match (string replace -rf '^  (\S+)( \S*)?$' '$1' -- $line)
            set name $match
        else if test -n "$name"
            # The line after each name and version is its description
            printf '%s\t%s\n' $name (string trim -- $line)
            set name
        end
    end
end

# Succeeds when the command is one of $argv and nothing has been typed after it yet.
# Every command reads its package, query or --help from the first argument only
function __makekeeper_first_arg
    set -l tokens (commandline -opc)
    test (count $tokens) -eq 2; and contains -- $tokens[2] $argv
end

# Succeeds when the command is one of $argv and its first argument has been given
function __makekeeper_after_first_arg
    set -l tokens (commandline -opc)
    test (count $tokens) -ge 3; and contains -- $tokens[2] $argv
end

set -l commands install uninstall update list info search sync fetch gui

# Commands
complete -c makekeeper -n __fish_use_subcommand -a install -d "Install from the catalog, a Git URL or a directory"
complete -c makekeeper -n __fish_use_subcommand -a uninstall -d "Uninstall a package"
complete -c makekeeper -n __fish_use_subcommand -a update -d "Update one or all Git-based packages"
complete -c makekeeper -n __fish_use_subcommand -a list -d "List installed packages"
complete -c makekeeper -n __fish_use_subcommand -a info -d "Show details about an installed package"
complete -c makekeeper -n __fish_use_subcommand -a search -d "Search the package catalog"
complete -c makekeeper -n __fish_use_subcommand -a sync -d "Download the latest package catalog"
complete -c makekeeper -n __fish_use_subcommand -a fetch -d "Show package statistics"
complete -c makekeeper -n __fish_use_subcommand -a gui -d "Launch the graphical interface"

# Global options
complete -c makekeeper -n __fish_use_subcommand -s h -l help -d "Show help"
complete -c makekeeper -n __fish_use_subcommand -s v -l version -d "Show version"

# Help for a single command
complete -c makekeeper -n "__makekeeper_first_arg $commands" -s h -l help -d "Show help for this command"

# install <NAME|URL|DIR> [--name NAME] [--override] [--yes]
complete -c makekeeper -n "__makekeeper_first_arg install" -a "(__makekeeper_catalog_packages)"
complete -c makekeeper -n "__makekeeper_first_arg install" -a "(__fish_complete_directories (commandline -ct) 'Local project')"
complete -c makekeeper -n "__makekeeper_after_first_arg install" -l name -x -d "Register under a custom name"
complete -c makekeeper -n "__makekeeper_after_first_arg install" -l override -d "Reinstall if already installed"
complete -c makekeeper -n "__makekeeper_after_first_arg install uninstall" -s y -l yes -d "Don't ask for confirmation"

# uninstall <NAME> [--yes], info <NAME>, update [NAME]
complete -c makekeeper -n "__makekeeper_first_arg uninstall info update" -a "(__makekeeper_installed_packages)"
