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

class Tui
{
public:

  Tui();
  ~Tui();

  void run();

private:

  void event_loop();
  bool is_colorterm() const;
  int ctrl_key(int const c) const;
  std::string check_window_size(std::size_t const width, std::size_t const height) const;

  bool const _colorterm;
  Readline _readline;
  Peaclock _peaclock;
};

#endif // TUI_HH
