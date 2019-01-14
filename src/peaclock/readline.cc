#include "peaclock/readline.hh"

#include "ob/string.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

Readline::Readline()
{
}

Readline::~Readline()
{
}

Readline& Readline::prompt(std::string const& str, std::vector<std::string> const& style)
{
  _prompt.str = str;
  _prompt.style = style;
  _prompt.fmt = aec::wrap(str, style);

  return *this;
}

std::string Readline::operator()(bool& is_running)
{
  // width and height of the terminal
  std::size_t width {0};
  std::size_t height {0};
  OB::Term::size(width, height);

  // reset input struct
  _input = {};

  auto const render_line = [&]()
  {
    std::string prompt {_prompt.fmt};

    if (_input.str.size() + 2 > width)
    {
      if (! _input.off)
      {
        _input.fmt += aec::wrap(">", _prompt.style);
      }
      else if (_input.off + width - 2 < _input.str.size())
      {
        prompt = aec::wrap("<", _prompt.style);
        _input.fmt += aec::wrap(">", _prompt.style);
      }
      else
      {
        prompt = aec::wrap("<", _prompt.style);
        _input.fmt = _input.fmt.substr(0, width - 2);
      }
    }

    std::cout
    << aec::cursor_hide
    << aec::cr
    << aec::erase_line
    << prompt
    << _input.fmt
    << aec::cursor_set(_input.idx + 2, height)
    << aec::cursor_show
    << std::flush;
  };

  char seq[3];

  char c {0};
  int num_read {0};

  bool loop {true};
  auto wait {std::chrono::milliseconds(50)};

  std::cout
  << _prompt.fmt
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
          _input.str.clear();
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
                  if (_input.off + _input.idx < _input.str.size())
                  {
                    if (_input.idx + 2 < width)
                    {
                      _input.str.erase(_input.off + _input.idx, 1);
                    }
                    else
                    {
                      _input.str.erase(_input.idx, 1);
                    }

                    _input.fmt = _input.str.substr(_input.off, width - 2);

                    render_line();

                    _history.idx = _history.val.size();
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

                    _input.fmt = _input.str.substr(_input.off, width - 2);

                    render_line();

                    _history.idx = _history.val.size();
                  }
                  else if (_input.str.empty())
                  {
                    // exit the command prompt
                    loop = false;
                    _input.str.clear();
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
                if (_history.idx)
                {
                  if (_history.idx == _history.val.size())
                  {
                    _input.buf = _input.str;
                  }

                  --_history.idx;
                  _input.str = _history.val.at(_history.idx);

                  if (_input.str.size() + 1 >= width)
                  {
                    _input.off = _input.str.size() - width + 2;
                    _input.idx = width - 2;
                    _input.fmt = _input.str.substr(_input.off, width - 2);
                  }
                  else
                  {
                    _input.idx = _input.str.size();
                    _input.fmt = _input.str;
                  }

                  render_line();
                }
                break;
              }

              case 'B':
              {
                // key_down
                // cycle forwards in history
                if (_history.idx < _history.val.size())
                {
                  ++_history.idx;
                  if (_history.idx == _history.val.size())
                  {
                    _input.str = _input.buf;
                  }
                  else
                  {
                    _input.str = _history.val.at(_history.idx);
                  }

                  if (_input.str.size() + 1 >= width)
                  {
                    _input.off = _input.str.size() - width + 2;
                    _input.idx = width - 2;
                    _input.fmt = _input.str.substr(_input.off, width - 2);
                  }
                  else
                  {
                    _input.idx = _input.str.size();
                    _input.fmt = _input.str;
                  }

                  render_line();
                }
                break;
              }

              case 'C':
              {
                // key_right
                // move cursor right
                if (_input.off + _input.idx < _input.str.size())
                {
                  if (_input.idx + 2 < width)
                  {
                    ++_input.idx;
                  }
                  else
                  {
                    ++_input.off;
                  }

                  _input.fmt = _input.str.substr(_input.off, width - 2);

                  render_line();
                }
                break;
              }

              case 'D':
              {
                // key_left
                // move cursor left
                if (_input.off || _input.idx)
                {
                  if (_input.idx)
                  {
                    --_input.idx;
                  }
                  else
                  {
                    --_input.off;
                  }

                  _input.fmt = _input.str.substr(_input.off, width - 2);

                  render_line();
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
        _input.str.clear();
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
        if (_input.off + _input.idx < _input.str.size())
        {
          if (_input.idx + 2 < width)
          {
            ++_input.idx;
          }
          else
          {
            ++_input.off;
          }

          _input.fmt = _input.str.substr(_input.off, width - 2);

          render_line();
        }
        break;
      }

      // ctrl-e
      if (static_cast<int>(c) == ctrl_key('e'))
      {
        // move cursor to end of line
        if (_input.off + _input.idx < _input.str.size())
        {
          if (_input.str.size() + 1 >= width)
          {
            _input.off = _input.str.size() - width + 2;
            _input.idx = width - 2;
            _input.fmt = _input.str.substr(_input.off, width - 2);
          }
          else
          {
            _input.idx = _input.str.size();
            _input.fmt = _input.str;
          }

          render_line();
        }
        break;
      }

      // ctrl-a
      if (static_cast<int>(c) == ctrl_key('a'))
      {
        // move cursor to start of line
        if (_input.idx || _input.off)
        {
          _input.idx = 0;
          _input.off = 0;

          if (_input.str.size() + 1 >= width)
          {
            _input.fmt = _input.str.substr(_input.off, width - 2);
          }
          else
          {
            _input.fmt = _input.str;
          }

          render_line();
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
        if (_input.off || _input.idx)
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

          _input.fmt = _input.str.substr(_input.off, width - 2);

          render_line();

          _history.idx = _history.val.size();
        }

        // exit the command prompt
        else if (_input.str.empty())
        {
          loop = false;
          _input.str.clear();
          break;
        }

        break;
      }

      // insert or append char to input buffer
      if (_input.idx + 2 < width)
      {
        _input.str.insert(_input.off + _input.idx, 1, c);
        ++_input.idx;
      }
      else if (_input.idx + 2 >= width)
      {
        _input.str.insert(_input.off + _input.idx, 1, c);
        ++_input.off;
      }
      else
      {
        _input.str += c;
        ++_input.off;
      }

      _input.fmt = _input.str.substr(_input.off, width - 2);

      render_line();

      // set history index to end
      _history.idx = _history.val.size();
    }

    if (num_read == 0)
    {
      std::this_thread::sleep_for(wait);
    }
  }

  // normalize input string
  auto res = normalize(_input.str);

  // add result to history
  add_history(res);

  return res;
}

void Readline::add_history(std::string const& str)
{
  if (! str.empty() && ! (! _history.val.empty() && _history.val.back() == str))
  {
    if (auto pos = std::find(_history.val.begin(), _history.val.end(), str); pos != _history.val.end())
    {
      _history.val.erase(pos);
    }
    _history.val.emplace_back(str);
  }

  _history.idx = _history.val.size();
}

int Readline::ctrl_key(int const c) const
{
  return (c & 0x1f);
}

std::string Readline::normalize(std::string const& str) const
{
  // trim leading and trailing whitespace
  // collapse sequential whitespace
  return OB::String::lowercase(std::regex_replace(
    OB::String::trim(str), std::regex("\\s+"),
    " ", std::regex_constants::match_not_null
  ));
}
