#ifndef PEACLOCK_HH
#define PEACLOCK_HH

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <sstream>
#include <vector>

class Peaclock
{
public:

  Peaclock();
  ~Peaclock();

  void render(std::size_t const width, std::size_t const height, std::ostringstream& buf);

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
      std::string active;
      std::string inactive;
    } style;
  } config;

private:

  std::string offset_width(std::size_t const width) const;
  std::string offset_height(std::size_t const height) const;

  void extract_digits(int num, int& t0, int& t1) const;
  void set_binary_clock(std::size_t col, int num);

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
};

#endif // PEACLOCK_HH
