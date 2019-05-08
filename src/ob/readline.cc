#include "ob/readline.hh"

#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <deque>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

#include <filesystem>
namespace fs = std::filesystem;

namespace OB
{

Readline& Readline::style(std::string const& style)
{
  _style.input = style;

  return *this;
}

Readline& Readline::prompt(std::string const& str, std::string const& style)
{
  _prompt.str = str;
  _style.prompt = style;
  _prompt.fmt = aec::wrap(str, style);

  return *this;
}

void Readline::refresh()
{
  _prompt.lhs = _prompt.fmt;
  _prompt.rhs.clear();

  if (_input.str.cols() + 2 > _width)
  {
    std::size_t pos {_input.off + _input.idx - 1};
    std::size_t cols {0};

    for (; pos != OB::Text::String::npos && cols < _width - 2; --pos)
    {
      cols += _input.str.at(pos).cols;
    }

    if (pos == OB::Text::String::npos)
    {
      pos = 0;
    }

    std::size_t end {_input.off + _input.idx - pos};
    _input.fmt.str(_input.str.substr(pos, end));

    if (_input.fmt.cols() > _width - 2)
    {
      while (_input.fmt.cols() > _width - 2)
      {
        _input.fmt.erase(0, 1);
      }

      _input.cur = _input.fmt.cols();

      if (_input.cur == OB::Text::String::npos)
      {
        _input.cur = 0;
      }

      _prompt.lhs = aec::wrap("<", _style.prompt);
    }
    else
    {
      _input.cur = _input.fmt.cols();

      if (_input.cur == OB::Text::String::npos)
      {
        _input.cur = 0;
      }

      while (_input.fmt.cols() <= _width - 2)
      {
        _input.fmt.append(std::string(_input.str.at(end++).str));
      }

      _input.fmt.erase(_input.fmt.size() - 1, 1);
    }

    if (_input.off + _input.idx < _input.str.size())
    {
      _prompt.rhs = aec::wrap(
        OB::String::repeat(_width - _input.fmt.cols() - 2, " ") + ">",
        _style.prompt);
    }
  }
  else
  {
    _input.fmt = _input.str;
    _input.cur = _input.fmt.cols(0, _input.idx);

    if (_input.cur == OB::Text::String::npos)
    {
      _input.cur = 0;
    }
  }

  std::cout
  << aec::cursor_hide
  << aec::cr
  << aec::erase_line
  << _style.input
  << OB::String::repeat(_width, " ")
  << aec::cr
  << _prompt.lhs
  << _style.input
  << _input.fmt
  << aec::clear
  << _prompt.rhs
  << aec::cursor_set(_input.cur + 2, _height)
  << aec::cursor_show
  << std::flush;
}

std::string Readline::operator()(bool& is_running)
{
  // update width and height of terminal
  OB::Term::size(_width, _height);

  // reset input struct
  _input = {};

  // input key as 32-bit char
  char32_t ch {0};

  // input key as utf8 string
  // contains 1-4 bytes
  std::string utf8;

  bool loop {true};
  bool save_input {true};
  bool clear_input {false};
  auto wait {std::chrono::milliseconds(50)};

  std::cout
  << aec::cr
  << aec::erase_line
  << _style.input
  << OB::String::repeat(_width, " ")
  << aec::cr
  << _prompt.fmt
  << std::flush;

  while (loop && is_running)
  {
    std::this_thread::sleep_for(wait);

    while ((ch = OB::Term::get_key(&utf8)) > 0)
    {
      switch (ch)
      {
        case OB::Term::Key::escape:
        {
          // exit the command prompt
          loop = false;
          clear_input = true;

          break;
        }

        case OB::Term::Key::tab:
        {
          // TODO add tab completion

          break;
        }

        case OB::Term::ctrl_key('c'):
        {
          // exit the command prompt
          loop = false;
          save_input = false;
          clear_input = true;

          break;
        }

        case OB::Term::ctrl_key('u'):
        {
          edit_clear();

          break;
        }

        case OB::Term::Key::newline:
        {
          // submit the input string
          loop = false;

          break;
        }

        case OB::Term::Key::up:
        case OB::Term::ctrl_key('p'):
        {
          hist_prev();

          break;
        }

        case OB::Term::Key::down:
        case OB::Term::ctrl_key('n'):
        {
          hist_next();

          break;
        }

        case OB::Term::Key::right:
        case OB::Term::ctrl_key('f'):
        {
          curs_right();

          break;
        }

        case OB::Term::Key::left:
        case OB::Term::ctrl_key('b'):
        {
          curs_left();

          break;
        }

        case OB::Term::Key::end:
        case OB::Term::ctrl_key('e'):
        {
          curs_end();

          break;
        }

        case OB::Term::Key::home:
        case OB::Term::ctrl_key('a'):
        {
          curs_begin();

          break;
        }

        case OB::Term::Key::delete_:
        case OB::Term::ctrl_key('d'):
        {
          loop = edit_delete();

          break;
        }

        case OB::Term::Key::backspace:
        case OB::Term::ctrl_key('h'):
        {
          loop = edit_backspace();

          break;
        }

        default:
        {
          if (ch < 0xF0000 && (ch == OB::Term::Key::space || OB::Text::is_graph(static_cast<std::int32_t>(ch))))
          {
            edit_insert(utf8);
          }

          break;
        }
      }
    }
  }

  auto res = normalize(_input.str);

  if (save_input)
  {
    hist_push(res);
    hist_save(res);
  }

  if (clear_input)
  {
    res.clear();
  }

  return res;
}

void Readline::curs_begin()
{
  // move cursor to start of line

  if (_input.idx || _input.off)
  {
    _input.idx = 0;
    _input.off = 0;

    refresh();
  }
}

void Readline::curs_end()
{
  // move cursor to end of line

  if (_input.str.empty())
  {
    return;
  }

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.str.cols() + 2 > _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::curs_left()
{
  // move cursor left

  if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      --_input.off;
    }
    else
    {
      --_input.idx;
    }

    refresh();
  }
}

void Readline::curs_right()
{
  // move cursor right

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.idx + 2 < _width)
    {
      ++_input.idx;
    }
    else
    {
      ++_input.off;
    }

    refresh();
  }
}

void Readline::edit_insert(std::string const& str)
{
  // insert or append char to input buffer

  auto size {_input.str.size()};
  _input.str.insert(_input.off + _input.idx, str);

  if (size != _input.str.size())
  {
    if (_input.idx + 2 < _width)
    {
      ++_input.idx;
    }
    else
    {
      ++_input.off;
    }
  }

  refresh();

  hist_reset();
}

void Readline::edit_clear()
{
  // clear line

  _input.idx = 0;
  _input.off = 0;
  _input.str.clear();

  refresh();

  hist_reset();
}

bool Readline::edit_delete()
{
  // erase char under cursor

  if (_input.str.empty())
  {
    _input.str.clear();

    return false;
  }

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.idx + 2 < _width)
    {
      _input.str.erase(_input.off + _input.idx, 1);
    }
    else
    {
      _input.str.erase(_input.idx, 1);
    }

    refresh();

    hist_reset();
  }
  else if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      _input.str.erase(_input.off + _input.idx - 1, 1);
      --_input.off;
    }
    else
    {
      --_input.idx;
      _input.str.erase(_input.idx, 1);
    }

    refresh();

    hist_reset();
  }

  return true;
}

bool Readline::edit_backspace()
{
  // erase char behind cursor

  if (_input.str.empty())
  {
    _input.str.clear();

    return false;
  }

  _input.str.erase(_input.off + _input.idx - 1, 1);

  if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      --_input.off;
    }
    else if (_input.idx)
    {
      --_input.idx;
    }

    refresh();

    hist_reset();
  }

  return true;
}

void Readline::hist_prev()
{
  // cycle backwards in history

  if (_history().empty() && _history.search().empty())
  {
    return;
  }

  bool bounds {_history.search().empty() ?
    (_history.idx < _history().size() - 1) :
    (_history.idx < _history.search().size() - 1)};

  if (bounds || _history.idx == History::npos)
  {
    if (_history.idx == History::npos)
    {
      _input.buf = _input.str;

      if (! _input.buf.empty())
      {
        hist_search(_input.buf);
      }
    }

    ++_history.idx;

    if (_history.search().empty())
    {
      // normal search
      _input.str = _history().at(_history.idx);
    }
    else
    {
      // fuzzy search
      _input.str = _history().at(_history.search().at(_history.idx).idx);
    }

    if (_input.str.size() + 1 >= _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.off = 0;
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::hist_next()
{
  // cycle forwards in history

  if (_history.idx != History::npos)
  {
    --_history.idx;

    if (_history.idx == History::npos)
    {
      _input.str = _input.buf;
    }
    else if (_history.search().empty())
    {
      // normal search
      _input.str = _history().at(_history.idx);
    }
    else
    {
      // fuzzy search
      _input.str = _history().at(_history.search().at(_history.idx).idx);
    }

    if (_input.str.size() + 1 >= _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.off = 0;
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::hist_reset()
{
  _history.search.clear();
  _history.idx = History::npos;
}

void Readline::hist_search(std::string const& str)
{
  _history.search.clear();

  OB::Text::String input {OB::Text::normalize_foldcase(
    std::regex_replace(OB::Text::trim(str), std::regex("\\s+"),
    " ", std::regex_constants::match_not_null))};

  if (input.empty())
  {
    return;
  }

  std::size_t idx {0};
  std::size_t count {0};
  std::size_t weight {0};
  std::string prev_hist {" "};
  std::string prev_input {" "};
  OB::Text::String hist;

  for (std::size_t i = 0; i < _history().size(); ++i)
  {
    hist.str(OB::Text::normalize_foldcase(_history().at(i)));

    if (hist.size() <= input.size())
    {
      continue;
    }

    idx = 0;
    count = 0;
    weight = 0;
    prev_hist = " ";
    prev_input = " ";

    for (std::size_t j = 0, seq = 0; j < hist.size(); ++j)
    {
      if (idx < input.size() &&
        hist.at(j).str == input.at(idx).str)
      {
        ++seq;
        count += 1;

        if (seq > 1)
        {
          count += 1;
        }

        if (prev_hist == " " && prev_input == " ")
        {
          count += 1;
        }

        prev_input = input.at(idx).str;
        ++idx;

        // short circuit to keep history order
        // comment out to search according to closest match
        if (idx == input.size())
        {
          break;
        }
      }
      else
      {
        seq = 0;
        weight += 2;

        if (prev_input == " ")
        {
          weight += 1;
        }
      }

      prev_hist = hist.at(j).str;
    }

    if (idx != input.size())
    {
      continue;
    }

    while (count && weight)
    {
      --count;
      --weight;
    }

    _history.search().emplace_back(weight, i);
  }

  std::sort(_history.search().begin(), _history.search().end(),
  [](auto const& lhs, auto const& rhs)
  {
    // default to history order if score is equal
    return lhs.score == rhs.score ? lhs.idx < rhs.idx : lhs.score < rhs.score;
  });
}

void Readline::hist_push(std::string const& str)
{
  if (! str.empty() && ! (! _history().empty() && _history().back() == str))
  {
    if (auto pos = std::find(_history().begin(), _history().end(), str); pos != _history().end())
    {
      _history().erase(pos);
    }

    _history().emplace_front(str);
  }

  hist_reset();
}

void Readline::hist_load(fs::path const& path)
{
  if (! path.empty())
  {
    std::ifstream ifile {path};

    if (ifile && ifile.is_open())
    {
      std::string line;

      while (std::getline(ifile, line))
      {
        hist_push(line);
      }
    }

    hist_open(path);
  }
}

void Readline::hist_save(std::string const& str)
{
  if (_history.file.is_open())
  {
    _history.file
    << str << "\n"
    << std::flush;
  }
}

void Readline::hist_open(fs::path const& path)
{
  _history.file.open(path, std::ios::app);

  if (! _history.file.is_open())
  {
    throw std::runtime_error("could not open file '" + path.string() + "'");
  }
}

std::string Readline::normalize(std::string const& str) const
{
  // trim leading and trailing whitespace
  // collapse sequential whitespace
  return std::regex_replace(OB::Text::trim(str), std::regex("\\s+"),
    " ", std::regex_constants::match_not_null);
}

} // namespace OB
