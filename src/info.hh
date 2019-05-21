#ifndef INFO_HH
#define INFO_HH

#include "ob/parg.hh"
using Parg = OB::Parg;

#include "ob/term.hh"
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <string_view>

inline void program_init(Parg& pg)
{
  pg.name("peaclock").version("0.3.1 (17.05.2019)");
  pg.description("A responsive and customizable clock, timer, and stopwatch for the terminal.");

  pg.usage("[--config-dir <dir>] [--config|-u <file>] [<file>] [--colour <on|off|auto>]");
  pg.usage("[--help|-h] [--colour <on|off|auto>]");
  pg.usage("[--version|-v] [--colour <on|off|auto>]");
  pg.usage("[--license] [--colour <on|off|auto>]");

  pg.info({"Key Bindings", {
    {"q, Q, <ctrl-c>", "quit the program"},
    {":", "enter the command prompt"},
    {"<esc>", "clear prompt status message"},
    {"<space>", "start/stop timer or stopwatch"},
    {"<backspace>", "clear timer or stopwatch"},
    {"w", "mode clock"},
    {"e", "mode timer"},
    {"r", "mode stopwatch"},
    {"W", "view date"},
    {"E", "view ascii"},
    {"R", "view digital"},
    {"T", "view binary"},
    {"Y", "view icon"},
    {"a", "toggle 24 hour time"},
    {"s", "toggle seconds"},
    {"d", "toggle date"},
    {"f", "toggle auto size"},
    {"g", "toggle auto ratio"},
    {"u", "adjust x y ratio with hjkl"},
    {"i", "adjust x y margin with hjkl"},
    {"o", "adjust x y padding with hjkl"},
    {"p", "adjust x y block with hjkl"},
    {"x", "adjust hsl active-fg with hjkl;'"},
    {"c", "adjust hsl inactive-fg with hjkl;'"},
    {"v", "adjust hsl colon-fg with hjkl;'"},
    {"b", "adjust hsl active-bg with hjkl;'"},
    {"n", "adjust hsl inactive-bg with hjkl;'"},
    {"m", "adjust hsl colon-bg with hjkl;'"},
    {",", "adjust hsl background with hjkl;'"},
    {".", "adjust hsl date with hjkl;'"},
    {"h", "decrease x or hue value"},
    {"j", "decrease y or saturation value"},
    {"k", "increase y or saturation value"},
    {"l", "increase x or hue value"},
    {";", "decrease luminosity"},
    {"'", "increase luminosity"},
    {"/", "clear toggled style"},
  }});

  pg.info({"Prompt Bindings", {
    {"<esc>, <ctrl-c>", "exit the prompt"},
    {"<enter>", "submit the input"},
    {"<ctrl-u>", "clear the prompt"},
    {"<up>, <ctrl-p>", "previous history value based on current input"},
    {"<down>, <ctrl-n>", "next history value based on current input"},
    {"<left>, <ctrl-b>", "move cursor left"},
    {"<right>, <ctrl-f>", "move cursor right"},
    {"<home>, <ctrl-a>", "move cursor to the start of the input"},
    {"<end>, <ctrl-e>", "move cursor to the end of the input"},
    {"<delete>, <ctrl-d>", "delete character under the cursor or delete previous character if cursor is at the end of the input"},
    {"<backspace>, <ctrl-h>", "delete previous character"},
  }});

  pg.info({"Commands", {
    {"quit", {
      {"", "quit the program"},
    }},
    {"exit", {
      {"", "quit the program"},
    }},
    {"help <search>", {
      {"", "search or view the help output"},
    }},
    {"mkconfig <file>", {
      {"", "create 'file' and write the current config settings to it"},
    }},
    {"mkconfig! <file>", {
      {"", "overwrite or create 'file' and write the current config settings to it"},
    }},
    {"mode <value>", {
      {"clock",
        "the view will display the current time"},
      {"timer",
        "the view will display the timer"},
      {"stopwatch",
        "the view will display the stopwatch"},
    }},
    {"view <value>", {
      {"date",
        "display only the date"},
      {"ascii",
        "display the ascii clock with the date"},
      {"digital",
        "display the digital clock with the date"},
      {"binary",
        "display the binary clock with the date"},
      {"icon",
        "display the icon with the date"},
    }},
    {"stopwatch <value>", {
      {"clear",
        "clear the stopwatch"},
      {"stop",
        "stop the stopwatch"},
      {"start",
        "start the stopwatch"},
      {"00h:00m:00s",
        "set the initial start time"},
    }},
    {"timer <value>", {
      {"clear",
        "clear the timer to the initial value"},
      {"stop",
        "stop the timer"},
      {"start",
        "start the timer"},
      {"00h:00m:00s",
        "set the initial start time"},
    }},
    {"rate-input <milliseconds>", {
      {"", "set the duration in milliseconds between reading user input"},
    }},
    {"rate-refresh <milliseconds>", {
      {"", "set the duration in milliseconds between redrawing the output"},
    }},
    {"rate-status <milliseconds>", {
      {"", "set the duration in milliseconds to display status messages"},
    }},
    {"locale <str>", {
      {"", "set the locale, for example 'en_CA.utf8', an empty string clears the value"},
    }},
    {"timezone <str>", {
      {"", "set the timezone, for example 'America/Vancouver', an empty string clears the value"},
    }},
    {"date <str>", {
      {"", "set the date format string, an empty string clears the value"},
    }},
    {"fill <str>", {
      {"", "set the string value used to fill the active, inactive, and colon blocks of the clock, an empty string clears the value"},
    }},
    {"fill-active <str>", {
      {"", "set the string value used to fill the active blocks of the clock, an empty string clears the value"},
    }},
    {"fill-inactive <str>", {
      {"", "set the string value used to fill the inactive blocks of the clock, an empty string clears the value"},
    }},
    {"fill-colon <str>", {
      {"", "set the string value used to fill the colon blocks of the clock, an empty string clears the value"},
    }},
    {"timer-exec <str>", {
      {"", "set the string value to be executed by a shell upon timer completion, an empty string clears the value"},
    }},
    {"toggle <value>", {
      {"block",
        "adjust x y block with hjkl"},
      {"padding",
        "adjust x y padding with hjkl"},
      {"margin",
        "adjust x y margin with hjkl"},
      {"ratio",
        "adjust x y ratio with hjkl"},
      {"active-fg",
        "adjust hsl active-fg with hjkl;'"},
      {"inactive-fg",
        "adjust hsl inactive-fg with hjkl;'"},
      {"colon-fg",
        "adjust hsl colon-fg with hjkl;'"},
      {"active-bg",
        "adjust hsl active-bg with hjkl;'"},
      {"inactive-bg",
        "adjust hsl inactive-bg with hjkl;'"},
      {"colon-bg",
        "adjust hsl colon-bg with hjkl;'"},
      {"background",
        "adjust hsl background with hjkl;'"},
      {"date",
        "adjust hsl date with hjkl;'"},
    }},
    {"block <value>", {
      {"x y", "set the x y block size, the width and height of an individual block composing the clock"},
    }},
    {"block-x <x>", {
      {"", "set the x block size"},
    }},
    {"block-y <y>", {
      {"", "set the y block size"},
    }},
    {"padding <value>", {
      {"x y", "set the x y padding size, the width and height of the space between each individual block composing the clock"},
    }},
    {"padding-x <x>", {
      {"", "set the x padding size"},
    }},
    {"padding-y <y>", {
      {"", "set the y padding size"},
    }},
    {"margin <value>", {
      {"x y", "set the x y margin size, the space around the outside of the clock from the edge of the terminal"},
    }},
    {"margin-x <x>", {
      {"", "set the x margin size"},
    }},
    {"margin-y <y>", {
      {"", "set the y margin size"},
    }},
    {"ratio <value>", {
      {"x y", "set the x y ratio size, auto adjust the clock to conform to a specific aspect ratio, keep in mind that a square ratio would be '2 1' due to a terminal character cell having a height around twice the size of its width"},
    }},
    {"ratio-x <x>", {
      {"", "set the x ratio size"},
    }},
    {"ratio-y <y>", {
      {"", "set the y ratio size"},
    }},
    {"date-padding <value>", {
      {"", "set the padding size between the date and the clock"},
    }},
    {"set <value> <on|off>", {
      {"hour-24",
        "use 24 hour time"},
      {"seconds",
        "display seconds"},
      {"date",
        "display the date"},
      {"auto-size",
        "auto size the clock to fill the screen, overrides the current x y block size"},
      {"auto-ratio",
        "auto size the clock to use the aspect ratio set by the command 'ratio', overrides the current x y block size and auto-size"},
    }},
    {"style <value> <#000-#fff|#000000-#ffffff|0-255|Colour|reverse|clear>", {
      {"active-fg",
        "set the style of the text set by the command 'fill' used to draw active blocks in the clock"},
      {"inactive-fg",
        "set the style of the text set by the command 'fill' used to draw inactive blocks in the clock"},
      {"colon-fg",
        "set the style of the text set by the command 'fill-colon' used to draw colon blocks in the clock"},
      {"active-bg",
        "set the style of the background used to draw active blocks in the clock"},
      {"inactive-bg",
        "set the style of the background used to draw inactive blocks in the clock"},
      {"colon-bg",
        "set the style of the background used to draw colon blocks in the clock"},
      {"background",
        "set the style of the background"},
      {"date",
        "set the style of the date"},
      {"text",
        "set the style of the text used in the command prompt"},
      {"prompt",
        "set the style of the command prompt symbol shown at the start of the line"},
      {"success",
        "set the style of the prompt status on success"},
      {"error",
        "set the style of the prompt status on error"},
    }},
  }});

  pg.info({"Colour", {
    {"", "The following is a list of 4-bit colours that can be used with the 'style' command."},
    {"", ""},
    {"", "black [bright]"},
    {"", "red [bright]"},
    {"", "green [bright]"},
    {"", "yellow [bright]"},
    {"", "blue [bright]"},
    {"", "magenta [bright]"},
    {"", "cyan [bright]"},
    {"", "white [bright]"},
  }});

  pg.info({"Files", {
    {"Config Directory (DIR)", "${HOME}/.peaclock"},
    {"History Directory", "DIR/history"},
    {"Config File", "DIR/config"},
    {"Command History File", "DIR/history/command"},
  }});

  pg.info({"Configuration", {
    {"", "Use '--config=<file>' to override the default config file."},
    {"", ""},
    {"", "Use '--config-dir=<dir>' to override the default config directory."},
    {"", ""},
    {"", "The config directory and config file must be created by the user."},
    {"", ""},
    {"", "The config file in the config directory must be named 'config'."},
    {"", ""},
    {"", "It is a plain text file that can contain any of the commands listed in the 'Commands' section of the '--help' output. Each command must be on its own line. Lines that begin with the '#' character are treated as comments."},
  }});

  pg.info({"Examples", {
    {"", "peaclock"},
    {"", "peaclock --config \"./path/to/config/file\""},
    {"", "peaclock --config-dir \"~/.config/peaclock\""},
    {"", "peaclock --help --colour=off"},
    {"", "peaclock --help"},
    {"", "peaclock --version"},
    {"", "peaclock --license"},
  }});

  pg.info({"Exit Codes", {
    {"0", "normal"},
    {"1", "error"},
  }});

  pg.info({"Repository", {
    {"", "https://github.com/octobanana/peaclock.git"},
  }});

  pg.info({"Homepage", {
    {"", "https://octobanana.com/software/peaclock"},
  }});

  pg.info({"Meta", {
    {"", "The version format is 'major.minor.patch (day.month.year)'."},
  }});

  pg.author("Brett Robinson (octobanana) <octobanana.dev@gmail.com>");

  // general flags
  pg.set("help,h", "Print the help output.");
  pg.set("version,v", "Print the program version.");
  pg.set("license", "Print the program license.");

  // options
  pg.set("config,u", "", "file", "Use the commands in the config file 'file' for initialization. All other initializations are skipped. To skip all initializations, use the special name 'NONE'.");
  pg.set("config-dir", "", "dir", "use 'dir' as the config directory. To skip all initializations, use the special name 'NONE'.");
  pg.set("colour", "auto", "on|off|auto", "Print the program info output with colour either on, off, or auto based on if stdout is a tty.");

  pg.set_pos();
}

inline bool program_color(std::string_view color)
{
  if (color == "on")
  {
    // color on
    return true;
  }

  if (color == "off")
  {
    // color off
    return false;
  }

  // color auto
  return OB::Term::is_term(STDOUT_FILENO);
}

inline int program_info(Parg& pg)
{
  // init info/options
  program_init(pg);

  // parse options
  auto const status {pg.parse()};

  // set output color choice
  pg.color(program_color(pg.get<std::string>("colour")));

  if (status < 0)
  {
    // an error occurred
    std::cerr
    << pg.usage()
    << "\n"
    << pg.error();

    return -1;
  }

  if (pg.get<bool>("help"))
  {
    // show help output
    std::cout << pg.help();

    return 1;
  }

  if (pg.get<bool>("version"))
  {
    // show version output
    std::cout << pg.version();

    return 1;
  }

  if (pg.get<bool>("license"))
  {
    // show license output
    std::cout << pg.license();

    return 1;
  }

  // success
  return 0;
}

#endif // INFO_HH
