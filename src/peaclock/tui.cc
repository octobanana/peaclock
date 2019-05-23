#include "peaclock/tui.hh"

#include "ob/num.hh"
#include "ob/algorithm.hh"
#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <utility>
#include <optional>
#include <limits>

#include <filesystem>
namespace fs = std::filesystem;

using std::string_literals::operator""s;

// bool to string
#define btos(x) ("off\0on"+4*!!(x))

Tui::Tui(Parg const& parg) :
  _pg {parg},
  _colorterm {OB::Term::is_colorterm()}
{
  _ctx.prompt.timeout = _ctx.prompt.rate.get() / _ctx.refresh_rate.get();
}

bool Tui::press_to_continue(std::string const& str, char32_t val)
{
  std::cerr
  << "Press " << str << " to continue";

  _term_mode.set_min(1);
  _term_mode.set_raw();

  bool res {false};
  char32_t key {0};
  if ((key = OB::Term::get_key()) > 0)
  {
    res = (val == 0 ? true : val == key);
  }

  _term_mode.set_cooked();

  std::cerr
  << aec::nl;

  return res;
}

void Tui::base_config(fs::path const& path)
{
  _ctx.base_config = path;
}

void Tui::load_config(fs::path const& path)
{
  // ignore config if path equals "NONE"
  if (path == "NONE")
  {
    return;
  }

  // buffer for error output
  std::ostringstream err;

  if (! path.empty() && fs::exists(path))
  {
    std::ifstream file {path};

    if (file.is_open())
    {
      std::string line;
      std::size_t lnum {0};

      while (std::getline(file, line))
      {
        // increase line number
        ++lnum;

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
            err << path.string() << ":" << lnum << ": " << res.value().second << "\n";
          }
        }
      }
    }
    else
    {
      err << "error: could not open config file '" << path.string() << "'\n";
    }
  }
  else
  {
    err << "error: the file '" << path.string() << "' does not exist\n";
  }

  if (! err.str().empty())
  {
    std::cerr << err.str();

    if (! press_to_continue("ENTER", '\n'))
    {
      throw std::runtime_error("aborted by user");
    }
  }
}

void Tui::load_hist_command(fs::path const& path)
{
  _readline.hist_load(path);
}

bool Tui::mkconfig(std::string path, bool overwrite)
{
  if (path.front() == '~')
  {
    path.replace(0, 1, std::getenv("HOME"));
  }

  fs::path fpath {path};

  if (fs::exists(fpath))
  {
    if (! fs::is_regular_file(fpath))
    {
      set_status(false, "error: '" + fpath.lexically_normal().string() + "' is not a regular file");

      return false;
    }

    if (! overwrite)
    {
      set_status(false, "error: file '" + fpath.lexically_normal().string() + "' already exists, use command 'mkconfig!' to overwrite");

      return false;
    }
  }

  std::ofstream file {fpath};

  if (! file.is_open())
  {
    set_status(false, "error: could not open file '" + fpath.lexically_normal().string() + "'");

    return false;
  }

  // timestamp
  std::time_t t = std::time(0);
  std::tm tm = *std::localtime(&t);

  // dump current state to file
  file
  << "# peaclock config\n"
  << "# file: " << fpath.lexically_normal().string() << "\n"
  << "# date: " << std::put_time(&tm, "%FT%TZ") << "\n\n"
  << "mode " << Peaclock::Mode::str(_peaclock.cfg.mode) << "\n"
  << "view " << Peaclock::View::str(_peaclock.cfg.view) << "\n"
  << "toggle " << Peaclock::Toggle::str(_peaclock.cfg.toggle) << "\n"
  << "stopwatch start\n"
  << "timer " << OB::Timer::sec_to_str(_peaclock.cfg.timer_seconds) << "\n"
  << "timer-exec '" << OB::String::escape(_peaclock.cfg.timer_exec) << "'\n"
  << "date '" << OB::String::escape(_peaclock.cfg.datefmt) << "'\n"
  << "locale '" << _peaclock.cfg.locale << "'\n"
  << "timezone '" << _peaclock.cfg.timezone << "'\n"
  << "fill-colon '" << OB::String::escape(_peaclock.cfg.fill_colon) << "'\n"
  << "fill-active '" << OB::String::escape(_peaclock.cfg.fill_active) << "'\n"
  << "fill-inactive '" << OB::String::escape(_peaclock.cfg.fill_inactive) << "'\n"
  << "rate-input " << _ctx.input_interval.str() << "\n"
  << "rate-status " << _ctx.prompt.rate.str() << "\n"
  << "rate-refresh " << _ctx.refresh_rate.str() << "\n"
  << "block " << _peaclock.cfg.x_block.str() << " " << _peaclock.cfg.y_block.str() << "\n"
  << "padding " << _peaclock.cfg.x_space.str() << " " << _peaclock.cfg.y_space.str() << "\n"
  << "margin " << _peaclock.cfg.x_border.str() << " " << _peaclock.cfg.y_border.str() << "\n"
  << "ratio " << _peaclock.cfg.x_ratio.str() << " " << _peaclock.cfg.y_ratio.str() << "\n"
  << "date-padding " << _peaclock.cfg.date_padding.str() << "\n"
  << "set date " << btos(_peaclock.cfg.date) << "\n"
  << "set seconds " << btos(_peaclock.cfg.seconds) << "\n"
  << "set hour-24 " << btos(_peaclock.cfg.hour_24) << "\n"
  << "set auto-size " << btos(_peaclock.cfg.auto_size) << "\n"
  << "set auto-ratio " << btos(_peaclock.cfg.auto_ratio) << "\n"
  << "style active-fg " << _peaclock.cfg.style.active_fg.key() << "\n"
  << "style active-bg " << _peaclock.cfg.style.active_bg.key() << "\n"
  << "style inactive-fg " << _peaclock.cfg.style.inactive_fg.key() << "\n"
  << "style inactive-bg " << _peaclock.cfg.style.inactive_bg.key() << "\n"
  << "style colon-fg " << _peaclock.cfg.style.colon_fg.key() << "\n"
  << "style colon-bg " << _peaclock.cfg.style.colon_bg.key() << "\n"
  << "style date " << _peaclock.cfg.style.date.key() << "\n"
  << "style text " << _ctx.style.text.key() << "\n"
  << "style background " << _ctx.style.background.key() << "\n"
  << "style prompt " << _ctx.style.prompt.key() << "\n"
  << "style success " << _ctx.style.success.key() << "\n"
  << "style error " << _ctx.style.error.key() << "\n"
  << std::flush;

  set_status(true, "config saved to '" + fpath.lexically_normal().string() + "'");

  return true;
}

void Tui::run()
{
  std::cout
  << aec::cursor_hide
  << aec::screen_push
  << aec::cursor_hide
  << aec::screen_clear
  << aec::cursor_home
  << aec::mouse_enable
  << std::flush;

  // set terminal mode to raw
  _term_mode.set_min(0);
  _term_mode.set_raw();

  // start the event loop
  event_loop();

  std::cout
  << aec::mouse_disable
  << aec::nl
  << aec::screen_pop
  << aec::cursor_show
  << std::flush;
}

void Tui::event_loop()
{
  while (_ctx.is_running)
  {
    // get the terminal width and height
    OB::Term::size(_ctx.width, _ctx.height);

    // check for correct screen size
    if (screen_size() != 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(_ctx.input_interval));

      char32_t key {0};
      if ((key = OB::Term::get_key()) > 0)
      {
        switch (key)
        {
          case 'q': case 'Q':
          case OB::Term::ctrl_key('c'):
          {
            _ctx.is_running = false;

            break;
          }

          default:
          {
            break;
          }
        }
      }

      continue;
    }

    // render new content
    clear();
    draw();
    refresh();

    int wait {_ctx.refresh_rate.get()};
    int tick {0};

    while (_ctx.is_running && wait)
    {
      if (wait > _ctx.input_interval.get())
      {
        tick = _ctx.input_interval.get();
        wait -= _ctx.input_interval.get();
      }
      else
      {
        tick = wait;
        wait = 0;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(tick));

      get_input();
    }
  }
}

void Tui::clear()
{
  // clear screen
  _ctx.buf
  << aec::cursor_home
  << _ctx.style.background;

  OB::Algorithm::for_each(_ctx.height,
  [&](auto) {
    OB::Algorithm::for_each(_ctx.width, [&](auto) {
      _ctx.buf << " "; });
    _ctx.buf << "\n";
  },
  [&](auto) {
    OB::Algorithm::for_each(_ctx.width, [&](auto) {
      _ctx.buf << " "; });
  });

  _ctx.buf
  << aec::clear
  << aec::cursor_home;
}

void Tui::refresh()
{
  // output buffer to screen
  std::cout
  << _ctx.buf.str()
  << std::flush;

  // clear output buffer
  _ctx.buf.str("");
}

void Tui::draw()
{
  draw_content();
  draw_prompt_message();
  draw_keybuf();
}

void Tui::draw_content()
{
  _ctx.buf
  << aec::cursor_save;

  // timer
  if (! _peaclock.timer && _peaclock.cfg.timer_notify)
  {
    _peaclock.cfg.timer_notify = false;

    if (! _peaclock.cfg.timer_exec.empty())
    {
      std::thread notify {[cmd = _peaclock.cfg.timer_exec]() {
        std::system(cmd.c_str());
      }};

      notify.detach();
    }
  }

  // render new content
  _peaclock.render(_ctx.width, _ctx.height, _ctx.buf);

  _ctx.buf
  << aec::clear
  << aec::cursor_load;
}

void Tui::draw_keybuf()
{
  if (_ctx.keys.empty())
  {
    return;
  }

  _ctx.buf
  << aec::cursor_save
  << aec::cursor_set(_ctx.width - 3, _ctx.height)
  << _ctx.style.background
  << "    "
  << aec::cursor_set(_ctx.width - 3, _ctx.height)
  << _ctx.style.text
  << aec::space;

  for (auto const& e : _ctx.keys)
  {
    if (OB::Text::is_print(static_cast<std::int32_t>(e.val)))
    {
      _ctx.buf
      << e.str;
    }
  }

  _ctx.buf
  << aec::space
  << aec::clear
  << aec::cursor_load;
}

void Tui::draw_prompt_message()
{
  // check if command prompt message is active
  if (_ctx.prompt.count > 0)
  {
    --_ctx.prompt.count;

    _ctx.buf
    << aec::cursor_save
    << aec::cursor_set(0, _ctx.height)
    << _ctx.style.background
    << _ctx.style.prompt
    << ">"
    << _ctx.style.prompt_status
    << _ctx.prompt.str.substr(0, _ctx.width - 5)
    << aec::cursor_load;
  }
}

void Tui::set_status(bool success, std::string const& msg)
{
  _ctx.style.prompt_status = success ? _ctx.style.success : _ctx.style.error;
  _ctx.prompt.str = msg;
  _ctx.prompt.count = _ctx.prompt.timeout;
}

void Tui::get_input()
{
  if ((_ctx.key.val = OB::Term::get_key(&_ctx.key.str)) > 0)
  {
    _ctx.keys.emplace_back(_ctx.key);

    switch (_ctx.keys.at(0).val)
    {
      // quit
      case 'q': case 'Q':
      {
        _ctx.is_running = false;
        _ctx.keys.clear();

        return;
      }

      case OB::Term::ctrl_key('c'):
      {
        _ctx.is_running = false;
        _ctx.keys.clear();

        return;
      }

      case OB::Term::Key::escape:
      {
        _ctx.prompt.count = 0;
        _ctx.keys.clear();

        break;
      }

      // command prompt
      case ':':
      {
        command_prompt();
        _ctx.keys.clear();

        break;
      }

      case 'a':
      {
        _peaclock.cfg.hour_24 = ! _peaclock.cfg.hour_24;
        set_status(true, "set hour-24 "s + btos(_peaclock.cfg.hour_24));

        break;
      }

      case 's':
      {
        _peaclock.cfg.seconds = ! _peaclock.cfg.seconds;
        set_status(true, "set seconds "s + btos(_peaclock.cfg.seconds));

        break;
      }

      case 'd':
      {
        _peaclock.cfg.date = ! _peaclock.cfg.date;
        set_status(true, "set date "s + btos(_peaclock.cfg.date));

        break;
      }

      case 'f':
      {
        _peaclock.cfg.auto_size = ! _peaclock.cfg.auto_size;
        set_status(true, "set auto-size "s + btos(_peaclock.cfg.auto_size));

        break;
      }

      case 'g':
      {
        _peaclock.cfg.auto_ratio = ! _peaclock.cfg.auto_ratio;
        set_status(true, "set auto-ratio "s + btos(_peaclock.cfg.auto_ratio));

        break;
      }

      case 'h':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::block:
          {
            --_peaclock.cfg.x_block;
            set_status(true, "block-x " + _peaclock.cfg.x_block.str());

            break;
          }

          case Peaclock::Toggle::padding:
          {
            --_peaclock.cfg.x_space;
            set_status(true, "padding-x " + _peaclock.cfg.x_space.str());

            break;
          }

          case Peaclock::Toggle::margin:
          {
            --_peaclock.cfg.x_border;
            set_status(true, "margin-x " + _peaclock.cfg.x_border.str());

            break;
          }

          case Peaclock::Toggle::ratio:
          {
            --_peaclock.cfg.x_ratio;
            set_status(true, "ratio-x " + _peaclock.cfg.x_ratio.str());

            break;
          }

          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.hue(_peaclock.cfg.style.active_fg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.active_fg.hue()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.hue(_peaclock.cfg.style.active_bg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.active_bg.hue()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.hue(_peaclock.cfg.style.inactive_fg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.hue()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.hue(_peaclock.cfg.style.inactive_bg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.hue()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.hue(_peaclock.cfg.style.colon_fg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.colon_fg.hue()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.hue(_peaclock.cfg.style.colon_bg.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.colon_bg.hue()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.hue(_peaclock.cfg.style.date.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.date.hue()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.hue(_ctx.style.background.hue() - 0.5);
            _peaclock.cfg.style.background.hue(_peaclock.cfg.style.background.hue() - 0.5);
            set_status(true, "hue " + OB::String::to_string(_ctx.style.background.hue()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case 'j':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::block:
          {
            --_peaclock.cfg.y_block;
            set_status(true, "block-y " + _peaclock.cfg.y_block.str());

            break;
          }

          case Peaclock::Toggle::padding:
          {
            --_peaclock.cfg.y_space;
            set_status(true, "padding-y " + _peaclock.cfg.y_space.str());

            break;
          }

          case Peaclock::Toggle::margin:
          {
            --_peaclock.cfg.y_border;
            set_status(true, "margin-y " + _peaclock.cfg.y_border.str());

            break;
          }

          case Peaclock::Toggle::ratio:
          {
            --_peaclock.cfg.y_ratio;
            set_status(true, "ratio-y " + _peaclock.cfg.y_ratio.str());

            break;
          }

          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.sat(_peaclock.cfg.style.active_fg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.active_fg.sat()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.sat(_peaclock.cfg.style.active_bg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.active_bg.sat()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.sat(_peaclock.cfg.style.inactive_fg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.sat()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.sat(_peaclock.cfg.style.inactive_bg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.sat()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.sat(_peaclock.cfg.style.colon_fg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.colon_fg.sat()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.sat(_peaclock.cfg.style.colon_bg.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.colon_bg.sat()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.sat(_peaclock.cfg.style.date.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.date.sat()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.sat(_ctx.style.background.sat() - 0.5);
            _peaclock.cfg.style.background.sat(_peaclock.cfg.style.background.sat() - 0.5);
            set_status(true, "sat " + OB::String::to_string(_ctx.style.background.sat()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case 'k':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::block:
          {
            ++_peaclock.cfg.y_block;
            set_status(true, "block-y " + _peaclock.cfg.y_block.str());

            break;
          }

          case Peaclock::Toggle::padding:
          {
            ++_peaclock.cfg.y_space;
            set_status(true, "padding-y " + _peaclock.cfg.y_space.str());

            break;
          }

          case Peaclock::Toggle::margin:
          {
            ++_peaclock.cfg.y_border;
            set_status(true, "margin-y " + _peaclock.cfg.y_border.str());

            break;
          }

          case Peaclock::Toggle::ratio:
          {
            ++_peaclock.cfg.y_ratio;
            set_status(true, "ratio-y " + _peaclock.cfg.y_ratio.str());

            break;
          }

          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.sat(_peaclock.cfg.style.active_fg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.active_fg.sat()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.sat(_peaclock.cfg.style.active_bg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.active_bg.sat()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.sat(_peaclock.cfg.style.inactive_fg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.sat()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.sat(_peaclock.cfg.style.inactive_bg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.sat()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.sat(_peaclock.cfg.style.colon_fg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.colon_fg.sat()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.sat(_peaclock.cfg.style.colon_bg.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.colon_bg.sat()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.sat(_peaclock.cfg.style.date.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_peaclock.cfg.style.date.sat()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.sat(_ctx.style.background.sat() + 0.5);
            _peaclock.cfg.style.background.sat(_peaclock.cfg.style.background.sat() + 0.5);
            set_status(true, "sat " + OB::String::to_string(_ctx.style.background.sat()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case 'l':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::block:
          {
            ++_peaclock.cfg.x_block;
            set_status(true, "block-x " + _peaclock.cfg.x_block.str());

            break;
          }

          case Peaclock::Toggle::padding:
          {
            ++_peaclock.cfg.x_space;
            set_status(true, "padding-x " + _peaclock.cfg.x_space.str());

            break;
          }

          case Peaclock::Toggle::margin:
          {
            ++_peaclock.cfg.x_border;
            set_status(true, "margin-x " + _peaclock.cfg.x_border.str());

            break;
          }

          case Peaclock::Toggle::ratio:
          {
            ++_peaclock.cfg.x_ratio;
            set_status(true, "ratio-x " + _peaclock.cfg.x_ratio.str());

            break;
          }

          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.hue(_peaclock.cfg.style.active_fg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.active_fg.hue()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.hue(_peaclock.cfg.style.active_bg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.active_bg.hue()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.hue(_peaclock.cfg.style.inactive_fg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.hue()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.hue(_peaclock.cfg.style.inactive_bg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.hue()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.hue(_peaclock.cfg.style.colon_fg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.colon_fg.hue()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.hue(_peaclock.cfg.style.colon_bg.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.colon_bg.hue()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.hue(_peaclock.cfg.style.date.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_peaclock.cfg.style.date.hue()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.hue(_ctx.style.background.hue() + 0.5);
            _peaclock.cfg.style.background.hue(_peaclock.cfg.style.background.hue() + 0.5);
            set_status(true, "hue " + OB::String::to_string(_ctx.style.background.hue()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case ';':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.lum(_peaclock.cfg.style.active_fg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.active_fg.lum()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.lum(_peaclock.cfg.style.active_bg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.active_bg.lum()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.lum(_peaclock.cfg.style.inactive_fg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.lum()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.lum(_peaclock.cfg.style.inactive_bg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.lum()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.lum(_peaclock.cfg.style.colon_fg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.colon_fg.lum()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.lum(_peaclock.cfg.style.colon_bg.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.colon_bg.lum()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.lum(_peaclock.cfg.style.date.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.date.lum()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.lum(_ctx.style.background.lum() - 0.5);
            _peaclock.cfg.style.background.lum(_peaclock.cfg.style.background.lum() - 0.5);
            set_status(true, "lum " + OB::String::to_string(_ctx.style.background.lum()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case '\'':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.lum(_peaclock.cfg.style.active_fg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.active_fg.lum()));

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.lum(_peaclock.cfg.style.active_bg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.active_bg.lum()));

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.lum(_peaclock.cfg.style.inactive_fg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.inactive_fg.lum()));

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.lum(_peaclock.cfg.style.inactive_bg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.inactive_bg.lum()));

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.lum(_peaclock.cfg.style.date.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.date.lum()));

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.lum(_peaclock.cfg.style.colon_fg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.colon_fg.lum()));

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.lum(_peaclock.cfg.style.colon_bg.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_peaclock.cfg.style.colon_bg.lum()));

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.lum(_ctx.style.background.lum() + 0.5);
            _peaclock.cfg.style.background.lum(_peaclock.cfg.style.background.lum() + 0.5);
            set_status(true, "lum " + OB::String::to_string(_ctx.style.background.lum()));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case '/':
      {
        switch (_peaclock.cfg.toggle)
        {
          case Peaclock::Toggle::active_fg:
          {
            _peaclock.cfg.style.active_fg.key("clear");
            set_status(true, "active-fg clear");

            break;
          }

          case Peaclock::Toggle::active_bg:
          {
            _peaclock.cfg.style.active_bg.key("clear");
            set_status(true, "active-bg clear");

            break;
          }

          case Peaclock::Toggle::inactive_fg:
          {
            _peaclock.cfg.style.inactive_fg.key("clear");
            set_status(true, "inactive-fg clear");

            break;
          }

          case Peaclock::Toggle::inactive_bg:
          {
            _peaclock.cfg.style.inactive_bg.key("clear");
            set_status(true, "inactive-bg clear");

            break;
          }

          case Peaclock::Toggle::colon_fg:
          {
            _peaclock.cfg.style.colon_fg.key("clear");
            set_status(true, "colon-fg clear");

            break;
          }

          case Peaclock::Toggle::colon_bg:
          {
            _peaclock.cfg.style.colon_bg.key("clear");
            set_status(true, "colon-bg clear");

            break;
          }

          case Peaclock::Toggle::date:
          {
            _peaclock.cfg.style.date.key("clear");
            set_status(true, "date clear");

            break;
          }

          case Peaclock::Toggle::background:
          {
            _ctx.style.background.key("clear");
            _peaclock.cfg.style.background.key("clear");
            set_status(true, "background clear");

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case 'p':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::block;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'o':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::padding;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'i':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::margin;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'u':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::ratio;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'x':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::active_fg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'c':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::inactive_fg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'v':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::colon_fg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'b':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::active_bg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'n':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::inactive_bg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'm':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::colon_bg;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case '.':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::background;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case ',':
      {
        _peaclock.cfg.toggle = Peaclock::Toggle::date;
        set_status(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));

        break;
      }

      case 'w':
      {
        _peaclock.cfg.mode = Peaclock::Mode::clock;
        set_status(true, "mode " + Peaclock::Mode::str(_peaclock.cfg.mode));

        break;
      }

      case 'e':
      {
        _peaclock.cfg.mode = Peaclock::Mode::timer;
        set_status(true, "mode " + Peaclock::Mode::str(_peaclock.cfg.mode));

        break;
      }

      case 'r':
      {
        _peaclock.cfg.mode = Peaclock::Mode::stopwatch;
        set_status(true, "mode " + Peaclock::Mode::str(_peaclock.cfg.mode));

        break;
      }

      case 'W':
      {
        _peaclock.cfg.date = true;
        _peaclock.cfg.view = Peaclock::View::date;
        set_status(true, "view " + Peaclock::View::str(_peaclock.cfg.view));

        break;
      }

      case 'E':
      {
        _peaclock.cfg.view = Peaclock::View::ascii;
        set_status(true, "view " + Peaclock::View::str(_peaclock.cfg.view));

        break;
      }

      case 'R':
      {
        _peaclock.cfg.view = Peaclock::View::digital;
        set_status(true, "view " + Peaclock::View::str(_peaclock.cfg.view));

        break;
      }

      case 'T':
      {
        _peaclock.cfg.view = Peaclock::View::binary;
        set_status(true, "view " + Peaclock::View::str(_peaclock.cfg.view));

        break;
      }

      case 'Y':
      {
        _peaclock.cfg.view = Peaclock::View::icon;
        set_status(true, "view " + Peaclock::View::str(_peaclock.cfg.view));

        break;
      }

      case ' ':
      {
        switch (_peaclock.cfg.mode)
        {
          case Peaclock::Mode::timer:
          {
            if (_peaclock.timer.seconds() >= _peaclock.cfg.timer_seconds)
            {
              _peaclock.timer.reset();
              _peaclock.cfg.timer_notify = false;
              set_status(true, "timer clear");
            }
            else
            {
              _peaclock.timer.toggle();
              set_status(true, "timer "s + (_peaclock.timer ? "start" : "stop"));
            }

            break;
          }

          case Peaclock::Mode::stopwatch:
          {
            _peaclock.stopwatch.toggle();
            set_status(true, "stopwatch "s + (_peaclock.stopwatch ? "start" : "stop"));

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      case OB::Term::Key::backspace:
      {
        switch (_peaclock.cfg.mode)
        {
          case Peaclock::Mode::timer:
          {
            _peaclock.timer.reset();
            _peaclock.cfg.timer_notify = false;
            set_status(true, "timer clear");

            break;
          }

          case Peaclock::Mode::stopwatch:
          {
            _peaclock.stopwatch.reset();
            set_status(true, "stopwatch clear");

            break;
          }

          default:
          {
            break;
          }
        }

        break;
      }

      default:
      {
        // ignore
        draw_keybuf();
        refresh();
        _ctx.keys.clear();

        return;
      }
    }

    clear();
    draw();
    refresh();
    _ctx.keys.clear();
  }

  while (OB::Term::get_key(&_ctx.key.str) > 0);
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

  auto const keys = OB::String::split(input, " ", 2);

  if (keys.empty())
  {
    return {};
  }

  // store the matches returned from OB::String::match
  std::optional<std::vector<std::string>> match_opt;

  // quit
  if (keys.size() == 1 && (keys.at(0) == "q" || keys.at(0) == "Q" ||
    keys.at(0) == "quit" || keys.at(0) == "Quit" || keys.at(0) == "exit"))
  {
    _ctx.is_running = false;
    return {};
  }

  else if (keys.at(0) == "h" || keys.at(0) == "help")
  {
    std::cout
    << aec::mouse_disable
    << aec::nl
    << aec::screen_pop
    << aec::cursor_show
    << std::flush;

    _term_mode.set_cooked();

    std::system(("$(which less) -ir" +
      (keys.size() > 1 ? " '+/" + keys.at(1) +
      (keys.size() > 2 ? " " + keys.at(2) : "") + "'" : "") +
      " <<'EOF'\n" + _pg.help() + "EOF").c_str());

    _term_mode.set_raw();

    std::cout
    << aec::cursor_hide
    << aec::screen_push
    << aec::cursor_hide
    << aec::screen_clear
    << aec::cursor_home
    << aec::mouse_enable
    << std::flush;
  }

  else if (keys.at(0) == "timer" && (match_opt = OB::String::match(input,
    std::regex("^timer(?:\\s+(clear|start|stop|(?:(?:\\d+Y)?:?(?:\\d+M)?:?(?:\\d+W)?:?(?:\\d+D)?:?(?:\\d+h)?:?(?:\\d+m)?:?(?:\\d+s)?)))?$"))))
  {
    auto const match = OB::String::trim(match_opt.value().at(1));

    if (match.empty())
    {
      return std::make_pair(true, "timer " + OB::Timer::sec_to_str(_peaclock.cfg.timer_seconds));
      // return std::make_pair(true, "timer " + OB::Timer::sec_to_str(_peaclock.cfg.timer_seconds - _peaclock.timer.seconds()));
    }

    if (match == "clear")
    {
      _peaclock.timer.reset();
      _peaclock.cfg.timer_notify = false;
    }
    else if (match == "start")
    {
      if (_peaclock.timer.seconds() >= _peaclock.cfg.timer_seconds)
      {
        _peaclock.timer.reset();
        _peaclock.cfg.timer_notify = false;
      }

      _peaclock.timer.start();
    }
    else if (match == "stop")
    {
      _peaclock.timer.stop();
    }
    else
    {
      _peaclock.timer.reset();
      _peaclock.cfg.timer_notify = false;
      _peaclock.cfg.timer_seconds = OB::Timer::str_to_sec(match);
    }
  }

  else if (keys.at(0) == "stopwatch" && (match_opt = OB::String::match(input,
    std::regex("^stopwatch(?:\\s+(clear|start|stop|(?:(?:\\d+Y)?:?(?:\\d+M)?:?(?:\\d+W)?:?(?:\\d+D)?:?(?:\\d+h)?:?(?:\\d+m)?:?(?:\\d+s)?)))?$"))))
  {
    auto const match = OB::String::trim(match_opt.value().at(1));

    if (match.empty())
    {
      return std::make_pair(true, "stopwatch " + _peaclock.stopwatch.str());
    }

    if (match == "clear")
    {
      _peaclock.stopwatch.reset();
    }
    else if (match == "start")
    {
      _peaclock.stopwatch.start();
    }
    else if (match == "stop")
    {
      _peaclock.stopwatch.stop();
    }
    else
    {
      _peaclock.stopwatch.str(match);
    }
  }

  else if (keys.at(0) == "timer-exec" && (match_opt = OB::String::match(input,
    std::regex("^timer-exec(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "timer-exec '" + OB::String::escape(_peaclock.cfg.timer_exec) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg.timer_exec = "";
      }
      else
      {
        _peaclock.cfg.timer_exec = OB::String::unescape(match.substr(1, match.size() - 2));
      }
    }
  }

  else if (keys.at(0) == "mkconfig" || keys.at(0) == "mkconfig!")
  {
    if (keys.size() == 2)
    {
      mkconfig(keys.at(1), keys.at(0).back() == '!');
    }
    else
    {
      return std::make_pair(false, "error: expected output file path");
    }
  }

  else if (keys.at(0) == "rate-input" && (match_opt = OB::String::match(input,
    std::regex("^rate-input(?:\\s+([0-9]+))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "rate-input " + _ctx.input_interval.str());
    }
    else
    {
      _ctx.input_interval = std::stoi(match);
    }
  }

  else if (keys.at(0) == "rate-refresh" && (match_opt = OB::String::match(input,
    std::regex("^rate-refresh(?:\\s+([0-9]+))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "rate-refresh " + _ctx.refresh_rate.str());
    }
    else
    {
      _ctx.refresh_rate = std::stoi(match);
      _ctx.prompt.timeout = _ctx.prompt.rate.get() / _ctx.refresh_rate.get();
    }
  }

  else if (keys.at(0) == "rate-status" && (match_opt = OB::String::match(input,
    std::regex("^rate-status(?:\\s+([0-9]+))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "rate-status " + _ctx.prompt.rate.str());
    }
    else
    {
      _ctx.prompt.rate = std::stoi(match);
      _ctx.prompt.timeout = _ctx.prompt.rate.get() / _ctx.refresh_rate.get();
    }
  }

  else if (keys.at(0) == "locale" && (match_opt = OB::String::match(input,
    std::regex("^locale(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "locale '" + _peaclock.cfg.locale + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg_locale("");
      }
      else if (! _peaclock.cfg_locale(match.substr(1, match.size() - 2)))
      {
        return std::make_pair(false, "error: invalid locale '" + match + "'");
      }
    }
  }

  else if (keys.at(0) == "timezone" && (match_opt = OB::String::match(input,
    std::regex("^timezone(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "timezone '" + _peaclock.cfg.timezone + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg_timezone("");
      }
      else if (! _peaclock.cfg_timezone(match.substr(1, match.size() - 2)))
      {
        return std::make_pair(false, "error: invalid timezone '" + match + "'");
      }
    }
  }

  else if (keys.at(0) == "date" && (match_opt = OB::String::match(input,
    std::regex("^date(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "date '" + OB::String::escape(_peaclock.cfg.datefmt) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg_datefmt("");
      }
      else
      {
        _peaclock.cfg_datefmt(OB::String::unescape(match.substr(1, match.size() - 2)));
      }
    }
  }

  else if (keys.at(0) == "fill" && (match_opt = OB::String::match(input,
    std::regex("^fill(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "fill '" + OB::String::escape(_peaclock.cfg.fill_active) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg.fill_active = "";
        _peaclock.cfg.fill_inactive = "";
        _peaclock.cfg.fill_colon = "";
      }
      else
      {
        _peaclock.cfg.fill_active = OB::String::unescape(match.substr(1, match.size() - 2));
        _peaclock.cfg.fill_inactive = _peaclock.cfg.fill_active;
        _peaclock.cfg.fill_colon = _peaclock.cfg.fill_active;
      }
    }
  }

  else if (keys.at(0) == "fill-active" && (match_opt = OB::String::match(input,
    std::regex("^fill-active(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "fill-active '" + OB::String::escape(_peaclock.cfg.fill_active) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg.fill_active = "";
      }
      else
      {
        _peaclock.cfg.fill_active = OB::String::unescape(match.substr(1, match.size() - 2));
      }
    }
  }

  else if (keys.at(0) == "fill-inactive" && (match_opt = OB::String::match(input,
    std::regex("^fill-inactive(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "fill-inactive '" + OB::String::escape(_peaclock.cfg.fill_inactive) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg.fill_inactive = "";
      }
      else
      {
        _peaclock.cfg.fill_inactive = OB::String::unescape(match.substr(1, match.size() - 2));
      }
    }
  }

  else if (keys.at(0) == "fill-colon" && (match_opt = OB::String::match(input,
    std::regex("^fill-colon(?:\\s+(?:(" + _ctx.rx.str + ")))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "fill-colon '" + OB::String::escape(_peaclock.cfg.fill_colon) + "'");
    }
    else
    {
      if (match.size() == 2)
      {
        _peaclock.cfg.fill_colon = "";
      }
      else
      {
        _peaclock.cfg.fill_colon = OB::String::unescape(match.substr(1, match.size() - 2));
      }
    }
  }

  else if (keys.at(0) == "mode" && (match_opt = OB::String::match(input,
    std::regex("^mode(?:\\s+(clock|timer|stopwatch))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "mode " + Peaclock::Mode::str(_peaclock.cfg.mode));
    }
    else
    {
      _peaclock.cfg.mode = Peaclock::Mode::enm(match);
    }
  }

  else if (keys.at(0) == "view" && (match_opt = OB::String::match(input,
    std::regex("^view(?:\\s+(date|ascii|digital|binary|icon))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "view " + Peaclock::View::str(_peaclock.cfg.view));
    }
    else
    {
      _peaclock.cfg.view = Peaclock::View::enm(match);
    }
  }

  else if (keys.at(0) == "toggle" && (match_opt = OB::String::match(input,
    std::regex("^toggle(?:\\s+(block|padding|margin|ratio|active-fg|inactive-fg|colon-fg|active-bg|inactive-bg|colon-bg|date|background))?$"))))
  {
    auto const match = match_opt.value().at(1);

    if (match.empty())
    {
      return std::make_pair(true, "toggle " + Peaclock::Toggle::str(_peaclock.cfg.toggle));
    }
    else
    {
      _peaclock.cfg.toggle = Peaclock::Toggle::enm(match);
    }
  }

  else if (keys.at(0) == "date-padding" && (match_opt = OB::String::match(input,
    std::regex("^date-padding(?:\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);

    if (x.empty())
    {
      return std::make_pair(true, "date-padding " + _peaclock.cfg.date_padding.str());
    }
    else
    {
      _peaclock.cfg.date_padding = std::stoul(x);
    }
  }

  else if (keys.at(0) == "block" && (match_opt = OB::String::match(input,
    std::regex("^block(?:\\s+([0-9]+)\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);
    auto const y = match_opt.value().at(2);

    if (x.empty() && y.empty())
    {
      return std::make_pair(true, "block " + _peaclock.cfg.x_block.str() + " " + _peaclock.cfg.y_block.str());
    }
    else
    {
      _peaclock.cfg.x_block = std::stoul(x);
      _peaclock.cfg.y_block = std::stoul(y);
    }
  }

  else if (keys.at(0) == "block-x" && (match_opt = OB::String::match(input,
    std::regex("^block-x(?:\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);

    if (x.empty())
    {
      return std::make_pair(true, "block-x " + _peaclock.cfg.x_block.str());
    }
    else
    {
      _peaclock.cfg.x_block = std::stoul(x);
    }
  }

  else if (keys.at(0) == "block-y" && (match_opt = OB::String::match(input,
    std::regex("^block-y(?:\\s+([0-9]+))?$"))))
  {
    auto const y = match_opt.value().at(1);

    if (y.empty())
    {
      return std::make_pair(true, "block-y " + _peaclock.cfg.y_block.str());
    }
    else
    {
      _peaclock.cfg.y_block = std::stoul(y);
    }
  }

  else if (keys.at(0) == "padding" && (match_opt = OB::String::match(input,
    std::regex("^padding(?:\\s+([0-9]+)\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);
    auto const y = match_opt.value().at(2);

    if (x.empty() && y.empty())
    {
      return std::make_pair(true, "padding " + _peaclock.cfg.x_space.str() + " " + _peaclock.cfg.y_space.str());
    }
    else
    {
      _peaclock.cfg.x_space = std::stoul(x);
      _peaclock.cfg.y_space = std::stoul(y);
    }
  }

  else if (keys.at(0) == "padding-x" && (match_opt = OB::String::match(input,
    std::regex("^padding-x(?:\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);

    if (x.empty())
    {
      return std::make_pair(true, "padding-x " + _peaclock.cfg.x_space.str());
    }
    else
    {
      _peaclock.cfg.x_space = std::stoul(x);
    }
  }

  else if (keys.at(0) == "padding-y" && (match_opt = OB::String::match(input,
    std::regex("^padding-y(?:\\s+([0-9]+))?$"))))
  {
    auto const y = match_opt.value().at(1);

    if (y.empty())
    {
      return std::make_pair(true, "padding-y " + _peaclock.cfg.y_space.str());
    }
    else
    {
      _peaclock.cfg.y_space = std::stoul(y);
    }
  }

  else if (keys.at(0) == "margin" && (match_opt = OB::String::match(input,
    std::regex("^margin(?:\\s+([0-9]+)\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);
    auto const y = match_opt.value().at(2);

    if (x.empty() && y.empty())
    {
      return std::make_pair(true, "margin " + _peaclock.cfg.x_border.str() + " " + _peaclock.cfg.y_border.str());
    }
    else
    {
      _peaclock.cfg.x_border = std::stoul(x);
      _peaclock.cfg.y_border = std::stoul(y);
    }
  }

  else if (keys.at(0) == "margin-x" && (match_opt = OB::String::match(input,
    std::regex("^margin-x(?:\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);

    if (x.empty())
    {
      return std::make_pair(true, "margin-x " + _peaclock.cfg.x_border.str());
    }
    else
    {
      _peaclock.cfg.x_border = std::stoul(x);
    }
  }

  else if (keys.at(0) == "margin-y" && (match_opt = OB::String::match(input,
    std::regex("^margin-y(?:\\s+([0-9]+))?$"))))
  {
    auto const y = match_opt.value().at(1);

    if (y.empty())
    {
      return std::make_pair(true, "margin-y " + _peaclock.cfg.y_border.str());
    }
    else
    {
      _peaclock.cfg.y_border = std::stoul(y);
    }
  }

  else if (keys.at(0) == "ratio" && (match_opt = OB::String::match(input,
    std::regex("^ratio(?:\\s+([0-9]+)\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);
    auto const y = match_opt.value().at(2);

    if (x.empty() && y.empty())
    {
      return std::make_pair(true, "ratio " + _peaclock.cfg.x_ratio.str() + " " + _peaclock.cfg.y_ratio.str());
    }
    else
    {
      _peaclock.cfg.x_ratio = std::stoul(x);
      _peaclock.cfg.y_ratio = std::stoul(y);
    }
  }

  else if (keys.at(0) == "ratio-x" && (match_opt = OB::String::match(input,
    std::regex("^ratio-x(?:\\s+([0-9]+))?$"))))
  {
    auto const x = match_opt.value().at(1);

    if (x.empty())
    {
      return std::make_pair(true, "ratio-x " + _peaclock.cfg.x_ratio.str());
    }
    else
    {
      _peaclock.cfg.x_ratio = std::stoul(x);
    }
  }

  else if (keys.at(0) == "ratio-y" && (match_opt = OB::String::match(input,
    std::regex("^ratio-y(?:\\s+([0-9]+))?$"))))
  {
    auto const y = match_opt.value().at(1);

    if (y.empty())
    {
      return std::make_pair(true, "ratio-y " + _peaclock.cfg.y_ratio.str());
    }
    else
    {
      _peaclock.cfg.y_ratio = std::stoul(y);
    }
  }

  else if (keys.size() >= 2 && keys.at(0) == "style")
  {
    if (keys.at(1) == "active-fg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style active-fg " + _peaclock.cfg.style.active_fg.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.active_fg = color;
    }

    else if (keys.at(1) == "active-bg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style active-bg " + _peaclock.cfg.style.active_bg.key());
      }

      OB::Color color {keys.at(2), OB::Color::Type::bg};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.active_bg = color;
    }

    else if (keys.at(1) == "inactive-fg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style inactive-fg " + _peaclock.cfg.style.inactive_fg.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.inactive_fg = color;
    }

    else if (keys.at(1) == "inactive-bg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style inactive-bg " + _peaclock.cfg.style.inactive_bg.key());
      }

      OB::Color color {keys.at(2), OB::Color::Type::bg};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.inactive_bg = color;
    }

    else if (keys.at(1) == "colon-fg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style colon-fg " + _peaclock.cfg.style.colon_fg.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.colon_fg = color;
    }

    else if (keys.at(1) == "colon-bg")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style colon-bg " + _peaclock.cfg.style.colon_bg.key());
      }

      OB::Color color {keys.at(2), OB::Color::Type::bg};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.colon_bg = color;
    }

    else if (keys.at(1) == "date")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style date " + _peaclock.cfg.style.date.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _peaclock.cfg.style.date = color;
    }

    else if (keys.at(1) == "text")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style text " + _ctx.style.text.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _ctx.style.text = color;
    }

    else if (keys.at(1) == "background")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style background " + _ctx.style.background.key());
      }

      OB::Color color {keys.at(2), OB::Color::Type::bg};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _ctx.style.background = color;
      _peaclock.cfg.style.background = color;
    }

    else if (keys.at(1) == "prompt")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style prompt " + _ctx.style.prompt.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _ctx.style.prompt = color;
    }

    else if (keys.at(1) == "success")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style success " + _ctx.style.success.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _ctx.style.success = color;
    }

    else if (keys.at(1) == "error")
    {
      if (keys.size() < 3)
      {
        return std::make_pair(true, "style error " + _ctx.style.error.key());
      }

      OB::Color color {keys.at(2)};

      if (! color)
      {
        return std::make_pair(false, "warning: unknown command '" + input + "'");
      }

      _ctx.style.error = color;
    }

    else
    {
      return std::make_pair(false, "warning: unknown command '" + input + "'");
    }
  }

  else if (keys.size() >= 2 && keys.at(0) == "set")
  {
    if (match_opt = OB::String::match(input,
      std::regex("^set\\s+date(?:\\s+(true|false|t|f|1|0|on|off))?$")))
    {
      auto const match = match_opt.value().at(1);

      if (match.empty())
      {
        return std::make_pair(true, "set date "s + btos(_peaclock.cfg.date));
      }
      else if ("true" == match || "t" == match || "1" == match || "on" == match)
      {
        _peaclock.cfg.date = true;
      }
      else
      {
        _peaclock.cfg.date = false;
      }
    }

    else if (match_opt = OB::String::match(input,
      std::regex("^set\\s+seconds(?:\\s+(true|false|t|f|1|0|on|off))?$")))
    {
      auto const match = match_opt.value().at(1);

      if (match.empty())
      {
        return std::make_pair(true, "set seconds "s + btos(_peaclock.cfg.seconds));
      }
      else if ("true" == match || "t" == match || "1" == match || "on" == match)
      {
        _peaclock.cfg.seconds = true;
      }
      else
      {
        _peaclock.cfg.seconds = false;
      }
    }

    else if (match_opt = OB::String::match(input,
      std::regex("^set\\s+hour-24(?:\\s+(true|false|t|f|1|0|on|off))?$")))
    {
      auto const match = match_opt.value().at(1);

      if (match.empty())
      {
        return std::make_pair(true, "set hour-24 "s + btos(_peaclock.cfg.hour_24));
      }
      else if ("true" == match || "t" == match || "1" == match || "on" == match)
      {
        _peaclock.cfg.hour_24 = true;
      }
      else
      {
        _peaclock.cfg.hour_24 = false;
      }
    }

    else if (match_opt = OB::String::match(input,
      std::regex("^set\\s+auto-size(?:\\s+(true|false|t|f|1|0|on|off))?$")))
    {
      auto const match = match_opt.value().at(1);

      if (match.empty())
      {
        return std::make_pair(true, "set auto-size "s + btos(_peaclock.cfg.auto_size));
      }
      else if ("true" == match || "t" == match || "1" == match || "on" == match)
      {
        _peaclock.cfg.auto_size = true;
      }
      else
      {
        _peaclock.cfg.auto_size = false;
      }
    }

    else if (match_opt = OB::String::match(input,
      std::regex("^set\\s+auto-ratio(?:\\s+(true|false|t|f|1|0|on|off))?$")))
    {
      auto const match = match_opt.value().at(1);

      if (match.empty())
      {
        return std::make_pair(true, "set auto-ratio "s + btos(_peaclock.cfg.auto_ratio));
      }
      else if ("true" == match || "t" == match || "1" == match || "on" == match)
      {
        _peaclock.cfg.auto_ratio = true;
      }
      else
      {
        _peaclock.cfg.auto_ratio = false;
      }
    }

    else
    {
      return std::make_pair(false, "warning: unknown command '" + input + "'");
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
  // reset prompt message count
  _ctx.prompt.count = 0;

  // set prompt style
  _readline.style(_ctx.style.text.value() + _ctx.style.background.value());
  _readline.prompt(":", _ctx.style.prompt.value() + _ctx.style.background.value());

  std::cout
  << aec::cursor_save
  << aec::cursor_set(0, _ctx.height)
  << aec::erase_line
  << aec::cursor_show
  << std::flush;

  // read user input
  auto input = _readline(_ctx.is_running);

  std::cout
  << aec::cursor_hide
  << aec::cursor_load
  << std::flush;

  if (auto const res = command(input))
  {
    set_status(res.value().first, res.value().second);
  }
}

int Tui::screen_size()
{
  bool width_invalid {_ctx.width < _ctx.width_min};
  bool height_invalid {_ctx.height < _ctx.height_min};

  if (width_invalid || height_invalid)
  {
    clear();

    _ctx.buf
    << _ctx.style.background
    << _ctx.style.error;

    if (width_invalid && height_invalid)
    {
      _ctx.buf
      << "Error: width "
      << _ctx.width
      << " (min "
      << _ctx.width_min
      << ") height "
      << _ctx.height
      << " (min "
      << _ctx.height_min
      << ")";
    }
    else if (width_invalid)
    {
      _ctx.buf
      << "Error: width "
      << _ctx.width
      << " (min "
      << _ctx.width_min
      << ")";
    }
    else
    {
      _ctx.buf
      << "Error: height "
      << _ctx.height
      << " (min "
      << _ctx.height_min
      << ")";
    }

    _ctx.buf
    << aec::clear;

    refresh();

    return 1;
  }

  return 0;
}
