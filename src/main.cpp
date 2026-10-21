#include "cli.h"
#include "config.h"
#include "commands/install.h"
#include "commands/uninstall.h"
#include "commands/update.h"
#include "commands/list.h"
#include "commands/search.h"
#include "commands/sync.h"
#include "commands/gui.h"

int main(int argc, char* argv[]) {
    makekeeper::cli::CLI& app = makekeeper::cli::CLI::instance();

    // Register commands
    app.register_command({"install", "Install a package", makekeeper::commands::install});
    app.register_command({"uninstall", "Uninstall a package", makekeeper::commands::uninstall});
    app.register_command({"update", "Update a package", makekeeper::commands::update});
    app.register_command({"list", "List installed packages", makekeeper::commands::list});
    app.register_command({"search", "Search packages", makekeeper::commands::search});
    app.register_command({"sync", "Sync package catalog", makekeeper::commands::sync});
    app.register_command({"gui", "Launch GUI", makekeeper::commands::gui});

    return app.run(argc, argv);
}
