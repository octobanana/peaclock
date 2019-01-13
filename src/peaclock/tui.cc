#include "peaclock/tui.hh"

#include "ob/string.hh"
#include "ob/algorithm.hh"

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cstddef>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <utility>
#include <optional>

Tui::Tui() :
  _colorterm {is_colorterm()}
{
  _peaclock.config.style.active = _colorterm ? aec::fg_true("#4feae7") : aec::fg_cyan;
  _peaclock.config.style.inactive = _colorterm ? aec::fg_true("#424854") : aec::fg_white;
}

Tui::~Tui()
{
}

bool Tui::is_colorterm() const
{
  auto const colorterm = OB::String::lowercase(OB::String::env_var("COLORTERM"));

  if (colorterm == "truecolor" || colorterm == "24bit")
  {
    return true;
  }

  return false;
}

void Tui::run()
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

void Tui::event_loop()
{
  // control when to exit the event loop
  bool is_running {true};

  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};

  // styles
  std::string const style_prompt {_colorterm ? aec::fg_true("#424854") : ""};
  std::string const style_error {aec::fg_red};

  // command prompt string
  _readline.prompt(":", std::vector {style_prompt});

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
      << aec::wrap("?", style_prompt)
      << aec::wrap(status.str.substr(0, width - 2), style_error)
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

    // render content to buffer
    _peaclock.render(width, height, buf);

    // output buffer
    std::cout
    << buf.str()
    << std::flush;

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
            auto input {_readline(is_running)};
            input = std::regex_replace
            (
              OB::String::trim(input), std::regex("\\s+"),
              " ", std::regex_constants::match_not_null
            );
            _readline.add_history(input);

            std::cout
            << aec::cursor_hide
            << aec::cr
            << aec::erase_line;

            // store the matches returned from OB::String::match
            std::optional<std::vector<std::string>> match_opt;

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
              _peaclock.config = {};
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+active\\s+(#?[0-9a-fA-F]{6})$")))
            {
              // 24-bit color
              auto const match = std::move(match_opt.value());

              _peaclock.config.style.active = aec::fg_true(match.at(1));
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+active\\s+([0-9]{1,3})$")))
            {
              // 8-bit color
              auto const match = std::move(match_opt.value());

              _peaclock.config.style.active = aec::fg_256(match.at(1));
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+active\\s+(black|red|green|yellow|blue|magenta|cyan|white)$")))
            {
              // 4-bit color
              auto const match = std::move(match_opt.value()).at(1);

              if ("black" == match)
              {
                _peaclock.config.style.active = aec::fg_black;
              }
              else if ("red" == match)
              {
                _peaclock.config.style.active = aec::fg_red;
              }
              else if ("green" == match)
              {
                _peaclock.config.style.active = aec::fg_green;
              }
              else if ("yellow" == match)
              {
                _peaclock.config.style.active = aec::fg_yellow;
              }
              else if ("blue" == match)
              {
                _peaclock.config.style.active = aec::fg_blue;
              }
              else if ("magenta" == match)
              {
                _peaclock.config.style.active = aec::fg_magenta;
              }
              else if ("cyan" == match)
              {
                _peaclock.config.style.active = aec::fg_cyan;
              }
              else if ("white" == match)
              {
                _peaclock.config.style.active = aec::fg_white;
              }
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+inactive\\s+(#?[0-9a-fA-F]{6})$")))
            {
              // 24-bit color
              auto const match = std::move(match_opt.value());

              _peaclock.config.style.inactive = aec::fg_true(match.at(1));
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+inactive\\s+([0-9]{1,3})$")))
            {
              // 8-bit color
              auto const match = std::move(match_opt.value());

              _peaclock.config.style.inactive = aec::fg_256(match.at(1));
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+inactive\\s+(black|red|green|yellow|blue|magenta|cyan|white)$")))
            {
              // 4-bit color
              auto const match = std::move(match_opt.value()).at(1);

              if ("black" == match)
              {
                _peaclock.config.style.inactive = aec::fg_black;
              }
              else if ("red" == match)
              {
                _peaclock.config.style.inactive = aec::fg_red;
              }
              else if ("green" == match)
              {
                _peaclock.config.style.inactive = aec::fg_green;
              }
              else if ("yellow" == match)
              {
                _peaclock.config.style.inactive = aec::fg_yellow;
              }
              else if ("blue" == match)
              {
                _peaclock.config.style.inactive = aec::fg_blue;
              }
              else if ("magenta" == match)
              {
                _peaclock.config.style.inactive = aec::fg_magenta;
              }
              else if ("cyan" == match)
              {
                _peaclock.config.style.inactive = aec::fg_cyan;
              }
              else if ("white" == match)
              {
                _peaclock.config.style.inactive = aec::fg_white;
              }
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+hour\\s+(12|24)$")))
            {
              auto const match = std::move(match_opt.value());

              _peaclock.config.hour_24 = (match.at(1) == "24");
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+char\\s+(.{1,4})$")))
            {
              auto const match = std::move(match_opt.value());

              _peaclock.config.symbol = match.at(1);
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+bold\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              if ("" == val || "true" == val || "t" == val || "1" == val || "on" == val)
              {
                _peaclock.config.style.bold = aec::bold;
              }
              else
              {
                _peaclock.config.style.bold.clear();
              }
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+compact\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _peaclock.config.compact =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+digital\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _peaclock.config.digital_clock =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );

              if (! _peaclock.config.digital_clock && ! _peaclock.config.binary_clock)
              {
                _peaclock.config.binary_clock = true;
              }
            }
            else if (match_opt = OB::String::match(input,
              std::regex("^set\\s+binary\\s+(true|false|t|f|1|0|on|off)$")))
            {
              auto const match = std::move(match_opt.value());
              auto const& val = match.at(1);

              _peaclock.config.binary_clock =
              (
                "" == val ||
                "true" == val ||
                "t" == val ||
                "1" == val ||
                "on" == val
              );

              if (! _peaclock.config.binary_clock && ! _peaclock.config.digital_clock)
              {
                _peaclock.config.digital_clock = true;
              }
            }
            else
            {
              status.str = input;
              std::cout
              << aec::wrap("?", style_prompt)
              << aec::wrap(status.str.substr(0, width - 2), style_error);
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

int Tui::ctrl_key(int c) const
{
  return (c & 0x1f);
}

std::string Tui::check_window_size(std::size_t width, std::size_t height) const
{
  std::size_t width_min {11};
  if (_peaclock.config.compact)
  {
    width_min = 8;
  }

  std::size_t height_min {6};
  if (! _peaclock.config.binary_clock)
  {
    height_min = 2;
  }
  else if (! _peaclock.config.digital_clock)
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
      << "Error: width "
      << width
      << " ("
      << width_min
      << " min) & height "
      << height
      << " ("
      << height_min
      << " min)"
      << "\n";
    }
    else if (width_invalid)
    {
      msg
      << "Error: width "
      << width
      << " ("
      << width_min
      << " min)"
      << "\n";
    }
    else
    {
      msg
      << "Error: height "
      << height
      << " ("
      << height_min
      << " min)"
      << "\n";
    }

    return msg.str();
  }

  return {};
}
