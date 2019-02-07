#ifndef TUI_HH
#define TUI_HH

#include "peaclock/readline.hh"
#include "peaclock/peaclock.hh"

#include "ob/string.hh"

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <vector>
#include <utility>
#include <optional>

class Tui
{
public:

  Tui();
  ~Tui();

  void run();
  void config(std::string const& custom_path = {});

private:

  void event_loop();
  std::optional<std::pair<bool, std::string>> command(std::string const& input);
  void command_prompt();
  int ctrl_key(int const c) const;
  std::string check_window_size(std::size_t const width, std::size_t const height) const;

  bool const _colorterm;
  Readline _readline;
  Peaclock _peaclock;

  struct
  {
    // current terminal size
    std::size_t width {0};
    std::size_t height {0};

    // control when to exit the event loop
    bool is_running {true};

    // copy of default config for the reset command
    Peaclock::Config config_clear;

    // prompt status
    struct
    {
      std::string str;
      int count {0};
      int timeout {3};
    } status;

    // style
    struct
    {
      std::string status;
      std::string prompt;
      std::string success;
      std::string error;
    } style;
  } _ctx;
};

#endif // TUI_HH
