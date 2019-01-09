#ifndef PEACLOCK_HH
#define PEACLOCK_HH

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

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

  int ctrl_key(int c);
  std::string readline(std::string const& prompt, bool& is_running);

  void extract_digits(int num, int& t0, int& t1);
  void set_binary_clock(std::size_t col, int num);

  std::string offset_width(std::size_t width);
  std::string offset_height(std::size_t height);

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

  // command prompt history buffer
  std::vector<std::string> _history;
  std::size_t _history_index;

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

  Config _config_clear;
};

#endif // PEACLOCK_HH
