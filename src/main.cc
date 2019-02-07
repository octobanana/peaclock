#include "ob/parg.hh"
using Parg = OB::Parg;

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include "peaclock/tui.hh"

#include <cstddef>

#include <string>
#include <iostream>

// prototypes
int program_options(Parg& pg);

int program_options(Parg& pg)
{
  pg.name("peaclock").version("0.1.7 (23.01.2019)");
  pg.description("A colourful binary clock for the terminal.");
  pg.usage("[--config 'path']");
  pg.usage("[-h|--help]");
  pg.usage("[-v|--version]");
  pg.usage("[--license]");
  pg.info("Key Bindings", {
    "q\n    quit the program",
    ":\n    enter the command prompt",
  });
  pg.info("Commands", {
    "q\n    quit the program",
    "reset\n    reset all settings",
    "set char <single_char>\n    set the binary clock character symbol",
    "set hour <12|24>\n    set 12 or 24 hour time",
    "set bold <on|off>\n    toggle bold style",
    "set compact <on|off>\n    toggle compact style",
    "set binary <on|off>\n    toggle binary clock",
    "set digital <on|off>\n    toggle digital clock",
    "set active <#000000-#ffffff|0-255|Colour>\n    set active colour to 24-bit, 8-bit, or 4-bit value",
    "set inactive <#000000-#ffffff|0-255|Colour>\n    set inactive colour to 24-bit, 8-bit, or 4-bit value",
  });
  pg.info("Colour", {
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white",
  });
  pg.info("Config", {
    "~/.config/ob/peaclock/config",
    "~/.ob/peaclock/config",
  });
  pg.info("Examples", {
    "peaclock",
    "peaclock --config './path/to/config'",
    "peaclock --help",
    "peaclock --version",
    "peaclock --license",
  });
  pg.info("Exit Codes", {"0 -> normal", "1 -> error"});
  pg.info("Repository", {
    "https://github.com/octobanana/peaclock.git",
  });
  pg.info("Homepage", {
    "https://octobanana.com/software/peaclock",
  });
  pg.author("Brett Robinson (octobanana) <octobanana.dev@gmail.com>");

  // general flags
  pg.set("help,h", "print the help output");
  pg.set("version,v", "print the program version");
  pg.set("license", "print the program license");

  // options
  pg.set("config", "", "path", "custom path to config file");

  int status {pg.parse()};

  if (status < 0)
  {
    std::cerr << pg.help() << "\n";
    std::cerr << "Error: " << pg.error() << "\n";

    auto const similar_names = pg.similar();
    if (similar_names.size() > 0)
    {
      std::cerr
      << "did you mean:\n";
      for (auto const& e : similar_names)
      {
        std::cerr
        << "  --" << e << "\n";
      }
    }

    return -1;
  }

  if (pg.get<bool>("help"))
  {
    std::cerr << pg.help();

    return 1;
  }

  if (pg.get<bool>("version"))
  {
    std::cerr << pg.name() << " v" << pg.version() << "\n";

    return 1;
  }

  if (pg.get<bool>("license"))
  {
    std::cerr << R"(MIT License

Copyright (c) 2019 Brett Robinson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.)" << "\n";

    return 1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  Parg pg {argc, argv};
  int pstatus {program_options(pg)};
  if (pstatus > 0) return 0;
  if (pstatus < 0) return 1;

  try
  {
    // init
    Tui tui;

    // load config file
    tui.config(pg.get("config"));

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
