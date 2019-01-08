#include "ob/parg.hh"
using Parg = OB::Parg;

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include "peaclock/peaclock.hh"

#include <cstddef>

#include <string>
#include <iostream>

// prototypes
int program_options(Parg& pg);

int program_options(Parg& pg)
{
  pg.name("peaclock").version("0.0.0 (06.01.2019)");
  pg.description("A colourful binary clock for the terminal.");
  pg.usage("[flags] [options] [--] [arguments]");
  pg.usage("[-v|--version]");
  pg.usage("[-h|--help]");
  pg.info("Examples", {
    "peaclock --help",
    "peaclock --version",
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

  // pg.set_stdin();
  // pg.set_pos();

  int status {pg.parse()};

  // if (status > 0 && pg.get_stdin().empty())
  // {
  //   std::cerr << pg.help() << "\n";
  //   std::cerr << "Error: " << "expected arguments" << "\n";

  //   return -1;
  // }

  if (status < 0)
  {
    std::cerr << pg.help() << "\n";
    std::cerr << "Error: " << pg.error() << "\n";

    auto similar_names = pg.similar();
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
    Peaclock clock;
    clock.run();
  }
  catch(std::exception const& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
  }
  catch(...)
  {
    std::cerr << "Error: an unexpected error occurred\n";
  }

  return 0;
}
