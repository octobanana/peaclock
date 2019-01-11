#include "peaclock/peaclock.hh"

#include "ob/string.hh"
#include "ob/algorithm.hh"

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cstddef>

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <utility>

Peaclock::Peaclock()
{
}

Peaclock::~Peaclock()
{
}

void Peaclock::run()
{
  // clear screen
  std::cout
  << aec::cursor_hide
  << aec::erase_screen
  << aec::cursor_home
  << std::flush;

  // set terminal mode to raw
  OB::Term::Mode term_mode;
  term_mode.set_raw();

  // start the event loop
  event_loop();

  // get height of terminal
  std::size_t height {0};
  OB::Term::height(height);

  // clear screen
  std::cout
  << aec::cursor_set(0, height);
  OB::Algorithm::for_each(height, [](auto) {
    std::cout
    << aec::erase_line
    << aec::cursor_up;
  });
  std::cout
  << aec::cursor_show
  << std::flush;
}

void Peaclock::event_loop()
{
  // control when to exit the event loop
  bool is_running {true};

  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};

  // command prompt string
  std::string prompt {aec::wrap(":", aec::fg_white)};

  // command prompt status
  struct Status
  {
    std::string str;
    int count {0};
    int timeout {2};
  } status;

  // output buffer
  std::ostringstream buf;

  // each loop iteration should take around 1 second
  while (is_running)
  {
    // clear output buffer
    buf.str("");

    // get the terminal width and height
    OB::Term::size(width, height);
    buf.str(check_window_size(width, height));

    if (! buf.str().empty())
    {
      std::cout
      << aec::cursor_set(0, height);
      OB::Algorithm::for_each(height, [](auto) {
        std::cout
        << aec::erase_line
        << aec::cursor_up;
      });
      std::cout
      << buf.str()
      << std::flush;
      goto handle_input;
    }

    // check if command prompt message is active
    if (status.count > 0)
    {
      // clear screen with command prompt message
      --status.count;
      std::cout
      << aec::cursor_set(0, height)
      << aec::wrap("?", aec::fg_white)
      << aec::wrap(status.str.substr(0, width - 2), aec::fg_red)
      << aec::cursor_up
      << aec::erase_up
      << aec::cursor_home
      << std::flush;
    }
    else
    {
      // clear screen
      std::cout
      << aec::cursor_set(0, height);
      OB::Algorithm::for_each(height, [](auto) {
        std::cout
        << aec::erase_line
        << aec::cursor_up;
      });
      std::cout
      << std::flush;
    }

    // handle draw
    {
      // get the current local time
      std::time_t time_raw = std::time(nullptr);
      std::tm* time_now = std::localtime(&time_raw);

      // set 12 or 24 hour time
      int hour {time_now->tm_hour};
      if (! _config.hour_24)
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

      if (_config.hour_24)
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
      if (_config.binary_clock)
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
            buf << aec::wrap(_config.symbol, {_config.style.active, _config.style.bold});
          }
          else
          {
            // digit is set to off
            buf << aec::wrap(_config.symbol, {_config.style.inactive, _config.style.bold});
          }

          if (col < 6)
          {
            if (_config.compact)
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
      if (_config.digital_clock)
      {
        buf << offset_width_str;

        for (std::size_t i {0}; i < _digital_clock.size(); ++i)
        {
          buf << aec::wrap(_digital_clock.at(i), {_config.style.active, _config.style.bold});

          if (_config.compact)
          {
            if (i == 1 || i == 3)
            {
              buf << aec::wrap(":", {_config.style.inactive, _config.style.bold});
            }
          }
          else
          {
            buf << " ";
          }
        }
      }

      // output buffer
      {
        std::cout
        << buf.str()
        << std::flush;
      }
    }

    // handle input
handle_input:
    {
      char c {0};
      int num_read {0};

      int loop {20};
      auto wait {std::chrono::milliseconds(50)};

      while (is_running && loop-- > 0)
      {
        while ((num_read = read(STDIN_FILENO, &c, 1)) == 1)
        {
          if (num_read == -1 && errno != EAGAIN)
          {
            throw std::runtime_error("read failed");
          }

          // ctrl-c
          if (static_cast<int>(c) == ctrl_key('c'))
          {
            is_running = false;
            break;
          }

          // quit
          else if (c == 'q' || c == 'Q')
          {
            is_running = false;
            break;
          }

          // command prompt
          else if (c == ':')
          {
            std::size_t x {0};
            std::size_t y {0};
            aec::cursor_get(x, y, false);

            std::cout
            << aec::cursor_set(0, height)
            << aec::erase_line
            << aec::cursor_show
            << std::flush;

            // reset prompt message count
            status.count = 0;

            // read user input
            auto input {readline(prompt, is_running)};
            input = std::regex_replace
            (
              OB::String::trim(input), std::regex("\\s+"),
              " ", std::regex_constants::match_not_null
            );
            readline.add_history(input);

            std::cout
            << aec::cursor_hide
            << aec::cr
            << aec::erase_line;

            if (! is_running)
            {
              is_running = false;
              loop = 0;
              break;
            }
            else if (input.empty())
            {
            }
            else if (input == "q" || input == "Q" || input == "quit" || input == "exit")
            {
              is_running = false;
              loop = 0;
              break;
            }
            else if (input == "reset")
            {
              _config = {};
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+active\\s+(#?[0-9a-fA-F]{6})$")))
            {
              auto const match = std::move(match_opt.value());

              _config.style.active = aec::fg_true(match.at(1));
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+inactive\\s+(#?[0-9a-fA-F]{6})$")))
            {
              auto const match = std::move(match_opt.value());

              _config.style.inactive = aec::fg_true(match.at(1));
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+hour\\s+(12|24)$")))
            {
              auto const match = std::move(match_opt.value());

              _config.hour_24 = (match.at(1) == "24");
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+char\\s+(.{1})$")))
            {
              auto const match = std::move(match_opt.value());

              _config.symbol = match.at(1);
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+bold\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              if ("" == val || "true" == val || "t" == val || "1" == val || "on" == val)
              {
                _config.style.bold = aec::bold;
              }
              else
              {
                _config.style.bold.clear();
              }
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+compact\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _config.compact =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+digital\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _config.digital_clock =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );

              if (! _config.digital_clock && ! _config.binary_clock)
              {
                _config.binary_clock = true;
              }
            }
            else if (auto match_opt = OB::String::match(input,
              std::regex("^set\\s+binary\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _config.binary_clock =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );

              if (! _config.binary_clock && ! _config.digital_clock)
              {
                _config.digital_clock = true;
              }
            }
            else
            {
              status.str = input;
              std::cout
              << aec::wrap("?", aec::fg_white)
              << aec::wrap(status.str.substr(0, width - 2), aec::fg_red);
              status.count = status.timeout;
            }

            std::cout
            << aec::cursor_set(x, y)
            << std::flush;

            loop = 0;
            break;
          }
        }

        std::this_thread::sleep_for(wait);
      }
    }
  }
}

int Peaclock::ctrl_key(int c) const
{
  return (c & 0x1f);
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

std::string Peaclock::offset_width(std::size_t width) const
{
  std::size_t cols {11};
  if (_config.compact)
  {
    cols = 8;
  }

  std::size_t x_offset {(width > cols) ? ((width - cols) / 2) : 0};

  return OB::String::repeat(" " , x_offset);
}

std::string Peaclock::offset_height(std::size_t height) const
{
  std::size_t rows {4};
  if (! _config.binary_clock)
  {
    rows = 0;
  }

  std::size_t y_offset {0};
  if (_config.binary_clock && _config.digital_clock)
  {
    y_offset = (height > (rows + 1)) ? (height - (rows + 1)) / 2 : 0;
  }
  else
  {
    y_offset = (height > rows) ? (height - rows) / 2 : 0;
  }

  return OB::String::repeat("\n" , y_offset);
}

std::string Peaclock::check_window_size(std::size_t width, std::size_t height) const
{
  std::size_t width_min {11};
  if (_config.compact)
  {
    width_min = 8;
  }

  std::size_t height_min {6};
  if (! _config.binary_clock)
  {
    height_min = 2;
  }
  else if (! _config.digital_clock)
  {
    height_min = 5;
  }

  bool width_invalid {width < width_min};
  bool height_invalid {height < height_min};

  if (width_invalid || height_invalid)
  {
    std::ostringstream msg;

    if (width_invalid && height_invalid)
    {
      msg
      << "Error: width ("
      << width_min
      << " min) & height ("
      << height_min
      << " min)"
      << "\n";
    }
    else if (width_invalid)
    {
      msg
      << "Error: width ("
      << width_min
      << " min)"
      << "\n";
    }
    else
    {
      msg
      << "Error: height ("
      << height_min
      << " min)"
      << "\n";
    }

    return msg.str();
  }

  return {};
}
