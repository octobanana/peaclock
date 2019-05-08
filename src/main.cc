#include "info.hh"

#include "ob/parg.hh"
using Parg = OB::Parg;

#include "ob/term.hh"
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

#include "peaclock/tui.hh"

#include <fcntl.h>
#include <unistd.h>

#include <cstddef>

#include <string>
#include <iostream>

#include <filesystem>
namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
  std::ios_base::sync_with_stdio(false);

  Parg pg {argc, argv};
  auto const pg_status {program_info(pg)};
  if (pg_status > 0) return 0;
  if (pg_status < 0) return 1;

  try
  {
    // init
    Tui tui;

    if (! OB::Term::is_term(STDOUT_FILENO))
    {
      throw std::runtime_error("stdout is not a tty");
    }

    if (! OB::Term::is_term(STDIN_FILENO))
    {
      // reset stdin
      int tty = open("/dev/tty", O_RDONLY);
      dup2(tty, STDIN_FILENO);
      close(tty);
    }

    // load files
    {
      // determine config directory
      // default to '~/.peaclock'
      fs::path config_dir {pg.find("config-dir") ?
        pg.get<fs::path>("config-dir") :
        fs::path(OB::Term::env_var("HOME") + "/." + pg.name())};

      if (config_dir != "NONE" &&
        fs::exists(config_dir) && fs::is_directory(config_dir))
      {
        // set config directory
        tui.base_config(config_dir);

        // check/create default directories
        fs::path history_dir {config_dir / fs::path("history")};

        if (! fs::exists(history_dir) || ! fs::is_directory(history_dir))
        {
          fs::create_directory(history_dir);
        }

        // load history files
        tui.load_hist_command(history_dir / fs::path("command"));

        // load config file
        tui.load_config(pg.find("config") ? pg.get<fs::path>("config") :
          config_dir / fs::path("config"));
      }
    }

    // start event loop
    tui.run();
  }
  catch(std::exception const& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  catch(...)
  {
    std::cerr << "Error: an unexpected error occurred\n";
    return 1;
  }

  return 0;
}
