#include "peaclock/peaclock.hh"

#include "ob/string.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>

Peaclock::Peaclock()
{
}

Peaclock::~Peaclock()
{
}

void Peaclock::render(std::size_t const width, std::size_t const height, std::ostringstream& buf)
{
  // get the current local time
  std::time_t time_raw = std::time(nullptr);
  std::tm* time_now = std::localtime(&time_raw);

  // set 12 or 24 hour time
  int hour {time_now->tm_hour};
  if (! config.hour_24)
  {
    if (hour > 12)
    {
      hour -= 12;
    }
    else if (hour == 0)
    {
      hour = 12;
    }
  }

  // extract the individual digits
  extract_digits(hour, _digital_clock.at(0), _digital_clock.at(1));
  extract_digits(time_now->tm_min, _digital_clock.at(2), _digital_clock.at(3));
  extract_digits(time_now->tm_sec, _digital_clock.at(4), _digital_clock.at(5));

  // set the binary clock
  _binary_clock = _binary_clock_clear;

  if (config.hour_24)
  {
    _binary_clock.at(12) = 0;
  }
  else
  {
    _binary_clock.at(12) = -1;
  }

  for (std::size_t col {0}; col < 6; ++col)
  {
    set_binary_clock(col, _digital_clock.at(col));
  }

  // get width and height offsets
  auto offset_width_str = offset_width(width);
  auto offset_height_str = offset_height(height);

  buf << offset_height_str;

  // draw binary clock
  if (config.binary_clock)
  {
    int col {1};
    for (auto const& e : _binary_clock)
    {
      if (col == 1)
      {
        buf << offset_width_str;
      }

      if (e == -1)
      {
        // blank placeholder
        buf << " ";
      }
      else if (e == 1)
      {
        // digit is set to on
        buf << aec::wrap(config.symbol, std::vector {config.style.active, config.style.bold});
      }
      else
      {
        // digit is set to off
        buf << aec::wrap(config.symbol, std::vector {config.style.inactive, config.style.bold});
      }

      if (col < 6)
      {
        if (config.compact)
        {
          if (col == 2 || col == 4)
          {
            buf << " ";
          }
        }
        else
        {
          buf << " ";
        }

        ++col;
      }
      else
      {
        col = 1;
        buf << "\n";
      }
    }
  }

  // draw digital clock
  if (config.digital_clock)
  {
    buf << offset_width_str;

    for (std::size_t i {0}; i < _digital_clock.size(); ++i)
    {
      buf << aec::wrap(_digital_clock.at(i), std::vector {config.style.active, config.style.bold});

      if (config.compact)
      {
        if (i == 1 || i == 3)
        {
          buf << aec::wrap(":", std::vector {config.style.inactive, config.style.bold});
        }
      }
      else
      {
        buf << " ";
      }
    }
  }
}

void Peaclock::extract_digits(int num, int& t0, int& t1) const
{
  if (num < 10)
  {
    t0 = 0;
    t1 = num;
  }
  else
  {
    t0 = num / 10;
    t1 = num % 10;
  }
}

void Peaclock::set_binary_clock(std::size_t col, int num)
{
  if (num >= 8)
  {
    num -= 8;
    _binary_clock.at(col + (0 * 6)) = 1;
  }

  if (num >= 4)
  {
    num -= 4;
    _binary_clock.at(col + (1 * 6)) = 1;
  }

  if (num >= 2)
  {
    num -= 2;
    _binary_clock.at(col + (2 * 6)) = 1;
  }

  if (num == 1)
  {
    --num;
    _binary_clock.at(col + (3 * 6)) = 1;
  }
}

std::string Peaclock::offset_width(std::size_t const width) const
{
  std::size_t cols {11};
  if (config.compact)
  {
    cols = 8;
  }

  std::size_t x_offset {(width > cols) ? ((width - cols) / 2) : 0};

  return OB::String::repeat(" " , x_offset);
}

std::string Peaclock::offset_height(std::size_t const height) const
{
  std::size_t rows {4};
  if (! config.binary_clock)
  {
    rows = 0;
  }

  std::size_t y_offset {0};
  if (config.binary_clock && config.digital_clock)
  {
    y_offset = (height > (rows + 1)) ? (height - (rows + 1)) / 2 : 0;
  }
  else
  {
    y_offset = (height > rows) ? (height - rows) / 2 : 0;
  }

  return OB::String::repeat("\n" , y_offset);
}
