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

Peaclock::Peaclock()
{
}

Peaclock::~Peaclock()
{
}

void Peaclock::run()
{
  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};

  // get the terminal width and height
  OB::Term::size(width, height);

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

  // get the terminal width and height
  OB::Term::size(width, height);

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

  // command prompt strings
  std::string prompt {aec::wrap(":", aec::fg_white)};
  std::string prompt_buf;
  int prompt_clear_sec {2};
  int prompt_clear {0};

  // output buffer
  std::ostringstream buf;

  // each loop iteration should take around 1 second
  while (is_running)
  {
    // get the terminal width and height
    OB::Term::size(width, height);

    // clear output buffer
    buf.str("");

    // check if command prompt message is active
    if (prompt_clear > 0)
    {
      // clear screen with command prompt message
      --prompt_clear;
      std::cout
      << aec::cursor_set(0, height)
      << aec::wrap("?", aec::fg_white)
      << aec::wrap(prompt_buf.substr(0, width - 2), aec::fg_red)
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
      if (! _config.hour_24 && hour > 12)
      {
        hour -= 12;
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

      buf << offset_height(height);

      // draw binary clock
      if (_config.binary_clock)
      {
        int col {1};
        for (auto const& e : _binary_clock)
        {
          if (col == 1)
          {
            buf << offset_width(width);
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
        buf << offset_width(width);

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

            // read user input
            auto input {OB::String::trim(readline(prompt, is_running))};

            std::cout
            << aec::cursor_hide
            << aec::cr
            << aec::erase_line;

            if (! input.empty() && ! (! _history.empty() && _history.back() == input))
            {
              if (auto pos = std::find(_history.begin(), _history.end(), input); pos != _history.end())
              {
                _history.erase(pos);
              }
              _history.emplace_back(input);
            }
            _history_index = _history.size();

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
              _config = _config_clear;
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
              prompt_buf = input;
              std::cout
              << aec::wrap("?", aec::fg_white)
              << aec::wrap(prompt_buf.substr(0, width - 2), aec::fg_red);
              prompt_clear = prompt_clear_sec;
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

int Peaclock::ctrl_key(int c)
{
  return (c & 0x1f);
}


std::string Peaclock::readline(std::string const& prompt, bool& is_running)
{
  struct Input
  {
    std::size_t idx {0};
    std::size_t off {0};
    std::string buf;
    std::string str;
    std::string fmt;
  } input;

  char seq[3];

  char c {0};
  int num_read {0};

  bool loop {true};
  auto wait {std::chrono::milliseconds(50)};

  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};
  OB::Term::size(width, height);

  std::cout
    << prompt
    << std::flush;

  while (loop && is_running)
  {
    while ((num_read = read(STDIN_FILENO, &c, 1)) == 1)
    {
      if (num_read == -1 && errno != EAGAIN)
      {
        throw std::runtime_error("read failed");
      }

      // esc / esc sequence
      if (static_cast<int>(c) == 27)
      {
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
        {
          // exit the command prompt
          loop = false;
          input.str.clear();
          break;
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1)
        {
          break;
        }

        if (seq[0] == '[')
        {
          if (seq[1] >= '0' && seq[1] <= '9')
          {
            if (read(STDIN_FILENO, &seq[2], 1) != 1)
            {
              break;
            }

            if (seq[2] == '~')
            {
              switch (seq[1])
              {
                case '3':
                {
                  // key_del
                  // erase char under cursor
                  if (input.off + input.idx < input.str.size())
                  {
                    if (input.idx + 2 < width)
                    {
                      input.str.erase(input.off + input.idx, 1);
                    }
                    else
                    {
                      input.str.erase(input.idx, 1);
                    }

                    input.fmt = input.str.substr(input.off, width - 2);

                    std::cout
                    << aec::cursor_hide
                    << aec::cr
                    << aec::erase_line
                    << prompt
                    << input.fmt
                    << aec::cursor_set(input.idx + 2, height)
                    << aec::cursor_show
                    << std::flush;

                    _history_index = _history.size();
                  }
                  else if (input.off || input.idx)
                  {
                    if (input.off)
                    {
                      input.str.erase(input.off + input.idx - 1, 1);
                      --input.off;
                    }
                    else
                    {
                      --input.idx;
                      input.str.erase(input.idx, 1);
                    }

                    input.fmt = input.str.substr(input.off, width - 2);

                    std::cout
                    << aec::cursor_hide
                    << aec::cr
                    << aec::erase_line
                    << prompt
                    << input.fmt
                    << aec::cursor_set(input.idx + 2, height)
                    << aec::cursor_show
                    << std::flush;

                    _history_index = _history.size();
                  }
                  else if (input.str.empty())
                  {
                    // exit the command prompt
                    loop = false;
                    input.str.clear();
                    break;
                  }

                  break;
                }

                default:
                {
                  break;
                }
              }

              break;
            }
          }
          else
          {
            switch (seq[1])
            {
              case 'A':
              {
                // key_up
                // cycle backwards in history
                if (_history_index)
                {
                  if (_history_index == _history.size())
                  {
                    input.buf = input.str;
                  }

                  --_history_index;
                  input.str = _history.at(_history_index);

                  if (input.str.size() + 1 >= width)
                  {
                    input.off = input.str.size() - width + 2;
                    input.idx = width - 2;
                    input.fmt = input.str.substr(input.off, width - 2);
                  }
                  else
                  {
                    input.idx = input.str.size();
                    input.fmt = input.str;
                  }

                  std::cout
                  << aec::cursor_hide
                  << aec::cr
                  << aec::erase_line
                  << prompt
                  << input.fmt
                  << aec::cursor_set(input.idx + 2, height)
                  << aec::cursor_show
                  << std::flush;
                }
                break;
              }

              case 'B':
              {
                // key_down
                // cycle forwards in history
                if (_history_index < _history.size())
                {
                  ++_history_index;
                  if (_history_index == _history.size())
                  {
                    input.str = input.buf;
                  }
                  else
                  {
                    input.str = _history.at(_history_index);
                  }

                  if (input.str.size() + 1 >= width)
                  {
                    input.off = input.str.size() - width + 2;
                    input.idx = width - 2;
                    input.fmt = input.str.substr(input.off, width - 2);
                  }
                  else
                  {
                    input.idx = input.str.size();
                    input.fmt = input.str;
                  }

                  std::cout
                  << aec::cursor_hide
                  << aec::cr
                  << aec::erase_line
                  << prompt
                  << input.fmt
                  << aec::cursor_set(input.idx + 2, height)
                  << aec::cursor_show
                  << std::flush;
                }
                break;
              }

              case 'C':
              {
                // key_right
                // move cursor right
                if (input.off + input.idx < input.str.size())
                {
                  if (input.idx + 2 < width)
                  {
                    ++input.idx;
                  }
                  else
                  {
                    ++input.off;
                    input.fmt = input.str.substr(input.off, width - 2);
                  }

                  std::cout
                  << aec::cursor_hide
                  << aec::cr
                  << aec::erase_line
                  << prompt
                  << input.fmt
                  << aec::cursor_set(input.idx + 2, height)
                  << aec::cursor_show
                  << std::flush;
                }
                break;
              }

              case 'D':
              {
                // key_left
                // move cursor left
                if (input.off || input.idx)
                {
                  if (input.idx)
                  {
                    --input.idx;
                  }
                  else
                  {
                    --input.off;
                    input.fmt = input.str.substr(input.off, width - 2);
                  }

                  std::cout
                  << aec::cursor_hide
                  << aec::cr
                  << aec::erase_line
                  << prompt
                  << input.fmt
                  << aec::cursor_set(input.idx + 2, height)
                  << aec::cursor_show
                  << std::flush;
                }
                break;
              }

              default:
              {
                break;
              }
            }

            break;
          }
        }

        break;
      }

      // ctrl-c
      if (static_cast<int>(c) == ctrl_key('c'))
      {
        // exit the main event loop
        is_running = false;
        input.str.clear();
        break;
      }

      // ctrl-d
      if (static_cast<int>(c) == ctrl_key('d'))
      {
        // submit the input string
        loop = false;
        break;
      }

      // ctrl-f
      if (static_cast<int>(c) == ctrl_key('f'))
      {
        // move cursor right
        if (input.off + input.idx < input.str.size())
        {
          if (input.idx + 2 < width)
          {
            ++input.idx;
          }
          else
          {
            ++input.off;
            input.fmt = input.str.substr(input.off, width - 2);
          }

          std::cout
          << aec::cursor_hide
          << aec::cr
          << aec::erase_line
          << prompt
          << input.fmt
          << aec::cursor_set(input.idx + 2, height)
          << aec::cursor_show
          << std::flush;
        }
        break;
      }

      // ctrl-e
      if (static_cast<int>(c) == ctrl_key('e'))
      {
        // move cursor to end of line
        if (input.off + input.idx < input.str.size())
        {
          if (input.str.size() + 1 >= width)
          {
            input.off = input.str.size() - width + 2;
            input.idx = width - 2;
            input.fmt = input.str.substr(input.off, width - 2);
          }
          else
          {
            input.idx = input.str.size();
            input.fmt = input.str;
          }

          std::cout
          << aec::cursor_hide
          << aec::cr
          << aec::erase_line
          << prompt
          << input.fmt
          << aec::cursor_set(input.idx + 2, height)
          << aec::cursor_show
          << std::flush;
        }
        break;
      }

      // ctrl-a
      if (static_cast<int>(c) == ctrl_key('a'))
      {
        // move cursor to start of line
        if (input.idx || input.off)
        {
          input.idx = 0;
          input.off = 0;

          if (input.str.size() + 1 >= width)
          {
            input.fmt = input.str.substr(input.off, width - 2);
          }
          else
          {
            input.fmt = input.str;
          }

          std::cout
          << aec::cursor_hide
          << aec::cr
          << aec::erase_line
          << prompt
          << input.fmt
          << aec::cursor_set(input.idx + 2, height)
          << aec::cursor_show
          << std::flush;
        }
        break;
      }

      // enter
      if (c == '\n')
      {
        // submit the input string
        loop = false;
        break;
      }

      // tab
      if (c == '\t')
      {
        break;
      }

      // backspace
      if (static_cast<int>(c) == 127 || static_cast<int>(c) == ctrl_key('h'))
      {
        // erase char behind cursor
        if (input.off || input.idx)
        {
          if (input.off)
          {
            input.str.erase(input.off + input.idx - 1, 1);
            --input.off;
          }
          else
          {
            --input.idx;
            input.str.erase(input.idx, 1);
          }

          input.fmt = input.str.substr(input.off, width - 2);

          std::cout
          << aec::cursor_hide
          << aec::cr
          << aec::erase_line
          << prompt
          << input.fmt
          << aec::cursor_set(input.idx + 2, height)
          << aec::cursor_show
          << std::flush;

          _history_index = _history.size();
        }

        // exit the command prompt
        else if (input.str.empty())
        {
          loop = false;
          input.str.clear();
          break;
        }

        break;
      }

      // insert or append char to input buffer
      if (input.idx + 2 < width)
      {
        input.str.insert(input.off + input.idx, 1, c);
        ++input.idx;
      }
      else if (input.idx + 2 >= width)
      {
        input.str.insert(input.off + input.idx, 1, c);
        ++input.off;
      }
      else
      {
        input.str += c;
        ++input.off;
      }

      input.fmt = input.str.substr(input.off, width - 2);

      // echo input buffer to stdout
      std::cout
      << aec::cursor_hide
      << aec::cr
      << aec::erase_line
      << prompt
      << input.fmt
      << aec::cursor_set(input.idx + 2, height)
      << aec::cursor_show
      << std::flush;

      // set history index to end
      _history_index = _history.size();

    }

    if (num_read == 0)
    {
      std::this_thread::sleep_for(wait);
    }
  }

  return input.str;
}

void Peaclock::extract_digits(int num, int& t0, int& t1)
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

std::string Peaclock::offset_width(std::size_t width)
{
  std::size_t cols {11};
  if (_config.compact)
  {
    cols = 8;
  }

  std::size_t x_offset {(width > cols) ? ((width - cols) / 2) : 0};

  return OB::String::repeat(" " , x_offset);
}

std::string Peaclock::offset_height(std::size_t height)
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
