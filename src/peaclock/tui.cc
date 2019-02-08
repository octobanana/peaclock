#include "peaclock/tui.hh"

#include "ob/util.hh"
#include "ob/string.hh"
#include "ob/algorithm.hh"

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cstddef>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <utility>
#include <optional>

Tui::Tui() :
  _colorterm {OB::Term::is_colorterm()}
{
  _peaclock.config.style.active = _colorterm ? aec::fg_true("#4feae7") : aec::fg_cyan;
  _peaclock.config.style.inactive = _colorterm ? aec::fg_true("#424854") : aec::fg_black;

  _readline.prompt(":", std::vector {_ctx.style.prompt});

  _ctx.style.prompt = _colorterm ? aec::fg_true("#424854") : "";
  _ctx.style.success = aec::fg_green;
  _ctx.style.error = aec::fg_red;
}

Tui::~Tui()
{
}

void Tui::config(std::string const& custom_path)
{
  // find if config file exists
  // custom_path
  // ${XDG_CONFIG_HOME}/ob/peaclock/config
  // ${HOME}/.ob/peaclock/config
  // none

  // ignore config if path equals "NONE"
  if (custom_path == "NONE")
  {
    return;
  }

  bool use_default {true};
  std::string path;

  // custom_path
  if (! custom_path.empty() && OB::Util::file_exists(custom_path))
  {
    use_default = false;
    path = custom_path;
  }

  if (use_default)
  {
    std::string home {OB::Term::env_var("HOME")};
    std::string config_home {OB::Term::env_var("XDG_CONFIG_HOME")};
    if (config_home.empty())
    {
      config_home = home + "/.config/ob/peaclock/config";
    }
    else
    {
      config_home += "/ob/peaclock/config";
    }

    // ${XDG_CONFIG_HOME}/ob/peaclock/config
    if (OB::Util::file_exists(config_home))
    {
      path = config_home;
    }

    // ${HOME}/.ob/peaclock/config
    else if (config_home = home + "/.ob/peaclock/config"; OB::Util::file_exists(config_home))
    {
      path = config_home;
    }
  }

  // custom path passed but does not exist
  if (use_default && ! custom_path.empty())
  {
    std::cerr << "warning: could not open config file '" << custom_path << "'\n";
  }

  // none
  if (path.empty())
  {
    return;
  }

  std::ifstream file {path};

  if (file && file.is_open())
  {
    std::string line;
    std::size_t num {0};

    while (std::getline(file, line))
    {
      // increase line number
      ++num;

      // trim leading and trailing whitespace
      line = OB::String::trim(line);

      // ignore empty line or comment
      if (line.empty() || OB::String::assert_rx(line, std::regex("^#[^\\r]*$")))
      {
        continue;
      }

      if (auto const res = command(line))
      {
        if (! res.value().first)
        {
          // source:line: level: info
          std::cerr << path << ":" << num << ": " << res.value().second << "\n";
        }
      }
    }
  }
  else
  {
    std::cerr << "warning: could not open config file '" << path << "'\n";
    return;
  }
}

void Tui::run()
{
  std::cout
  << aec::cursor_hide
  << aec::screen_push
  << aec::cursor_hide
  << aec::screen_clear
  << aec::cursor_home
  << std::flush;

  // set terminal mode to raw
  OB::Term::Mode term_mode;
  term_mode.set_raw();

  // start the event loop
  event_loop();

  std::cout
  << aec::nl
  << aec::screen_pop
  << aec::cursor_show
  << std::flush;
}

void Tui::event_loop()
{
  _ctx.config_clear = _peaclock.config;

  // output buffer
  std::ostringstream buf;

  // each loop iteration should take around 1 second
  while (_ctx.is_running)
  {
    // clear output buffer
    buf.str("");

    // get the terminal width and height
    OB::Term::size(_ctx.width, _ctx.height);
    buf.str(check_window_size(_ctx.width, _ctx.height));

    if (! buf.str().empty())
    {
      std::cout
      << aec::cursor_set(0, _ctx.height);
      OB::Algorithm::for_each(_ctx.height, [](auto) {
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
    if (_ctx.status.count > 0)
    {
      // clear screen with command prompt message
      --_ctx.status.count;
      std::cout
      << aec::cursor_set(0, _ctx.height)
      << aec::wrap("?", _ctx.style.prompt)
      << aec::wrap(_ctx.status.str.substr(0, _ctx.width - 2), _ctx.style.status)
      << aec::cursor_up
      << aec::erase_up
      << aec::cursor_home
      << std::flush;
    }
    else
    {
      // clear screen
      std::cout
      << aec::cursor_set(0, _ctx.height);
      OB::Algorithm::for_each(_ctx.height, [](auto) {
        std::cout
        << aec::erase_line
        << aec::cursor_up;
      });
      std::cout
      << std::flush;
    }

    // render content to buffer
    _peaclock.render(_ctx.width, _ctx.height, buf);

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

      while (_ctx.is_running && loop-- > 0)
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
            _ctx.is_running = false;
            break;
          }

          // quit
          else if (c == 'q' || c == 'Q')
          {
            _ctx.is_running = false;
            break;
          }

          // command prompt
          else if (c == ':')
          {
            command_prompt();
            loop = 0;
            break;
          }
        }

        std::this_thread::sleep_for(wait);
      }
    }
  }
}

std::optional<std::pair<bool, std::string>> Tui::command(std::string const& input)
{
  // quit
  if (! _ctx.is_running)
  {
    _ctx.is_running = false;
    return {};
  }

  // nop
  if (input.empty())
  {
    return {};
  }

  // store the matches returned from OB::String::match
  std::optional<std::vector<std::string>> match_opt;

  // quit
  if (match_opt = OB::String::match(input,
    std::regex("^\\s*(q|Q|quit|Quit)\\s*$")))
  {
    _ctx.is_running = false;
    return {};
  }
  else if (input == "reset")
  {
    _peaclock.config = _ctx.config_clear;
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

  // unknown
  else
  {
    return std::make_pair(false, "warning: unknown command '" + input + "'");
  }

  return {};
}

void Tui::command_prompt()
{
  std::cout
  << aec::cursor_save
  << aec::cursor_set(0, _ctx.height)
  << aec::erase_line
  << aec::cursor_show
  << std::flush;

  // reset prompt message count
  _ctx.status.count = 0;

  // read user input
  auto input = _readline(_ctx.is_running);

  std::cout
  << aec::cursor_hide
  << aec::cr
  << aec::erase_line
  << std::flush;

  if (auto const res = command(input))
  {
    _ctx.style.status = res.value().first ? _ctx.style.success : _ctx.style.error;
    _ctx.status.str = res.value().second;
    std::cout
    << aec::wrap("?", _ctx.style.prompt)
    << aec::wrap(_ctx.status.str.substr(0, _ctx.width - 2), _ctx.style.status);
    _ctx.status.count = _ctx.status.timeout;
  }

  std::cout
  << aec::cursor_load
  << std::flush;
}

int Tui::ctrl_key(int const c) const
{
  return (c & 0x1f);
}

std::string Tui::check_window_size(std::size_t const width, std::size_t const height) const
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
