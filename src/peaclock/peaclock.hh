#ifndef PEACLOCK_HH
#define PEACLOCK_HH

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include "peaclock/readline.hh"

#include <cstddef>

#include <string>
#include <vector>

class Peaclock
{
public:

  Peaclock();
  ~Peaclock();

  void run();

private:

  void event_loop();

  int ctrl_key(int c) const;

  void extract_digits(int num, int& t0, int& t1) const;
  void set_binary_clock(std::size_t col, int num);

  std::string offset_width(std::size_t width) const;
  std::string offset_height(std::size_t height) const;

  std::string check_window_size(std::size_t width, std::size_t height) const;

  Readline readline;

  using Clock = std::vector<int>;

  Clock _digital_clock
  {
      0, 0,  0, 0,  0, 0
   // H  h   M  m   S  s
  };

  Clock _binary_clock
  {
     -1, 0, -1, 0, -1, 0,
     -1, 0,  0, 0,  0, 0,
      0, 0,  0, 0,  0, 0,
      0, 0,  0, 0,  0, 0,
   // H  h   M  m   S  s
  };

  Clock const _binary_clock_clear = _binary_clock;

  struct Config
  {
    bool hour_24 {true};
    bool compact {true};
    bool binary_clock {true};
    bool digital_clock {true};

    std::string symbol {"|"};

    struct Style
    {
      std::string bold {aec::bold};
      std::string active {aec::fg_true("#4feae7")};
      std::string inactive {aec::fg_true("#424854")};
    } style;
  } _config;
};

#endif // PEACLOCK_HH
