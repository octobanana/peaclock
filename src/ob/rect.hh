#ifndef OB_RECT_HH
#define OB_RECT_HH

#include "ob/color.hh"
#include "ob/text.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>

namespace OB
{

class Rect
{
public:

  enum class Align
  {
    center,
    top,
    bottom,
    left,
    right
  };

  Rect() = default;
  Rect(Rect&&) = default;
  Rect(Rect const&) = default;
  ~Rect() = default;

  Rect& operator=(Rect&&) = default;
  Rect& operator=(Rect const&) = default;

  friend std::ostream& operator<<(std::ostream& os, Rect& obj)
  {
    obj.draw(os);

    return os;
  }

  Rect& draw(std::ostream& os)
  {
    bool _border {false};

    if (_border_top || _border_bottom || _border_left || _border_right)
    {
      _border = true;
    }

    bool _padding {false};

    if (_padding_top || _padding_bottom || _padding_left || _padding_right)
    {
      _padding = true;
    }

    auto padding_top = _padding_top;
    auto padding_bottom = _padding_bottom;
    auto padding_left = _padding_left;
    auto padding_right = _padding_right;

    if (_border && _padding)
    {
      if (_border_top)
      {
        ++padding_top;
      }

      if (_border_bottom)
      {
        ++padding_bottom;
      }

      if (_border_left)
      {
        ++padding_left;
      }

      if (_border_right)
      {
        ++padding_right;
      }
    }

    std::size_t text_width {_w - _border_left - _border_right - _padding_left - _padding_right};
    std::size_t text_height {_h - _border_top - _border_bottom - _padding_top - _padding_bottom};

    std::size_t row {0};
    auto const ln = OB::String::split(_text, "\n");
    OB::Text::View lnv;

    os
    << aec::clear
    << _color_fg
    << _color_bg;
    bool style_main {true};

    std::size_t y_begin {0};

    // determine y axis alignment
    switch (_y_align)
    {
      case Align::center:
      {
        if (text_height / 2 >= ln.size() / 2)
        {
          y_begin = text_height / 2 - ln.size() / 2;
        }

        break;
      }

      case Align::bottom:
      {
        if (text_height >= ln.size())
        {
          y_begin = text_height - ln.size();
        }

        break;
      }

      case Align::top:
      default:
      {
        break;
      }
    }

    for (std::size_t y = 0; y < _h && y + _y <= _y_max; ++y)
    {
      os
      << aec::cursor_set(_x, y + _y);

      std::size_t cursor_right {0};

      for (std::size_t x = 0; x < _w && x + _x <= _x_max; ++x)
      {
        if (_border)
        {
          if (y == 0 || x == 0)
          {
            if (y == 0 && x == 0 && (_border_top || _border_left))
            {
              if (style_main)
              {
                os
                << _border_fg
                << _border_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << _corner_top_left;

              cursor_right = 0;

              continue;
            }
            else if (y == 0 && x + 1 == _w && (_border_top || _border_right))
            {
              if (style_main)
              {
                os
                << _border_fg
                << _border_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << _corner_top_right;

              cursor_right = 0;

              continue;
            }
            else if (y + 1 == _h && x == 0 && (_border_bottom || _border_left))
            {
              if (style_main)
              {
                os
                << _border_fg
                << _border_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << _corner_bottom_left;

              cursor_right = 0;

              continue;
            }
            else if (y == 0 && _border_top)
            {
              if (style_main)
              {
                os
                << _border_fg
                << _border_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << _line_top;

              cursor_right = 0;

              continue;
            }
            else if (x == 0 && _border_left)
            {
              if (style_main)
              {
                os
                << _border_fg
                << _border_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << _line_left;

              cursor_right = 0;

              continue;
            }
          }
          else if (y + 1 == _h && x + 1 == _w && (_border_bottom || _border_right))
          {
            if (style_main)
            {
              os
              << _border_fg
              << _border_bg;
              style_main = ! style_main;
            }

            os
            << aec::cursor_right(cursor_right)
            << _corner_bottom_right;

            cursor_right = 0;

            continue;
          }
          else if (y + 1 == _h && _border_bottom)
          {
            if (style_main)
            {
              os
              << _border_fg
              << _border_bg;
              style_main = ! style_main;
            }

            os
            << aec::cursor_right(cursor_right)
            << _line_bottom;

            cursor_right = 0;

            continue;
          }
          else if (x + 1 == _w && _border_right)
          {
            if (style_main)
            {
              os
              << _border_fg
              << _border_bg;
              style_main = ! style_main;
            }

            os
            << aec::cursor_right(cursor_right)
            << _line_right;

            cursor_right = 0;

            continue;
          }
        }

        if (! style_main)
        {
          os
          << _color_fg.step()
          << _color_bg;
          style_main = ! style_main;
        }

        if (_padding)
        {
          if (y < padding_top)
          {
            ++cursor_right;

            continue;
          }

          if (y + padding_bottom >= _h)
          {
            ++cursor_right;

            continue;
          }

          if (x < padding_left)
          {
            ++cursor_right;

            continue;
          }

          if (x + padding_right >= _w)
          {
            ++cursor_right;

            continue;
          }
        }

        if (! _text.empty() && y >= y_begin && row < ln.size())
        {
          // set the view to the current line
          lnv.str(ln.at(row++));

          // line syntax highlighting
          syntax(lnv);

          // total columns in the current line
          auto tcols = lnv.cols();

          // total columns used so far
          std::size_t cols {0};

          int x_begin {0};

          // determine x axis alignment
          switch (_x_align)
          {
            case Align::center:
            {
              if (text_width / 2 >= tcols / 2)
              {
                x_begin = text_width / 2 - tcols / 2;
              }

              break;
            }

            case Align::right:
            {
              if (text_width >= tcols)
              {
                x_begin = text_width - tcols;
              }

              break;
            }

            case Align::left:
            default:
            {
              break;
            }
          }

          while (x_begin-- > 0)
          {
            ++cursor_right;
            ++cols;
          }

          os << aec::cursor_right(cursor_right);

          cursor_right = 0;

          {
            lnv.str(lnv.colstr(0, tcols <= text_width ? text_width : text_width - 1));
            cols += lnv.cols();

            std::size_t pos_line {0};
            std::size_t pos_syntax {0};

            for (auto const& e : lnv)
            {
              // TODO does not handle when syntax pos has multiple equal values
              if (pos_syntax < _syntax.size() && _syntax.at(pos_syntax).first == pos_line)
              {
                os << _highlight.at(_syntax.at(pos_syntax).second).second << e << aec::clear;
                ++pos_syntax;
                style_main = false;
              }
              else
              {
                if (! style_main)
                {
                  os
                  << _color_fg
                  << _color_bg;
                  style_main = ! style_main;
                }

                os << e;

                if (_color_fg.mode() == OB::Color::Mode::party)
                {
                  _color_fg.step();
                }
              }

              ++pos_line;
            }

            if (tcols <= text_width)
            {
              while (cols < text_width)
              {
                ++cursor_right;
                ++cols;
              }
            }
            else
            {
              while (cols + 1 < text_width)
              {
                ++cursor_right;
                ++cols;
              }

              if (! style_main)
              {
                os
                << _color_fg
                << _color_bg;
                style_main = ! style_main;
              }

              os
              << aec::cursor_right(cursor_right)
              << ">";

              cursor_right = 0;

              ++cols;
            }
          }

          // subtract 1 to counter the for loops ++x condition
          x += cols - 1;
        }
        else
        {
          if (_fill == " " && _color_bg.key() == "clear")
          {
            ++cursor_right;
          }
          else
          {
            os
            << aec::cursor_right(cursor_right)
            << _color_fg.step()
            << _fill;

            cursor_right = 0;
          }
        }
      }
    }

    return *this;
  }

  Rect& highlight(std::vector<std::pair<std::string, OB::Color>> const& hl)
  {
    _highlight = hl;

    return *this;
  }

  Rect& align(Align x, Align y)
  {
    _x_align = x;
    _y_align = y;

    return *this;
  }

  Rect& xy(std::size_t x, std::size_t y)
  {
    _x = ++x;
    _y = ++y;

    return *this;
  }

  Rect& xy_max(std::size_t x, std::size_t y)
  {
    _x_max = x;
    _y_max = y;

    return *this;
  }

  Rect& wh(std::size_t w, std::size_t h)
  {
    _w = w;
    _h = h;

    return *this;
  }

  Rect& fill(std::string const& str)
  {
    _fill = str;

    return *this;
  }

  Rect& color_fg(OB::Color const& color)
  {
    _color_fg = color;

    return *this;
  }

  Rect& color_bg(OB::Color const& color)
  {
    _color_bg = color;

    return *this;
  }

  Rect& text(std::string const& val)
  {
    _text = val;

    return *this;
  }

  Rect& border(bool top, bool right, bool bottom, bool left)
  {
    _border_top = top;
    _border_bottom = bottom;
    _border_left = left;
    _border_right = right;

    return *this;
  }

  Rect& border_fg(OB::Color const& color)
  {
    _border_fg = color;

    return *this;
  }

  Rect& border_bg(OB::Color const& color)
  {
    _border_bg = color;

    return *this;
  }

  Rect& padding(std::size_t top, std::size_t right, std::size_t bottom, std::size_t left)
  {
    _padding_top = top;
    _padding_bottom = bottom;
    _padding_left = left;
    _padding_right = right;

    return *this;
  }

private:

  void syntax(OB::Text::View const& view)
  {
    _syntax.clear();

    std::size_t pos {0};
    OB::Text::Regex search;

    for (auto const& [regex, color] : _highlight)
    {
      search.match(regex, view.str());

      if (! search.empty())
      {
        for (auto const& match : search)
        {
          _syntax.emplace_back(view.byte_to_char(match.pos), pos);
        }
      }

      ++pos;
    }

    std::sort(_syntax.begin(), _syntax.end(),
    [](auto const& lhs, auto const& rhs)
    {
      return lhs.first < rhs.first;
    });
  }

  std::size_t _x {0};
  std::size_t _y {0};

  std::size_t _x_max {0};
  std::size_t _y_max {0};

  Align _x_align {Align::left};
  Align _y_align {Align::top};

  std::size_t _w {0};
  std::size_t _h {0};

  std::string _fill {" "};

  OB::Color _color_fg {OB::Color::Type::fg};
  OB::Color _color_bg {OB::Color::Type::bg};

  // text
  OB::Text::String _text;
  std::vector<std::pair<std::size_t, std::size_t>> _syntax;
  std::vector<std::pair<std::string, OB::Color>> _highlight;

  // border
  bool _border_top {false};
  bool _border_bottom {false};
  bool _border_left {false};
  bool _border_right {false};
  OB::Color _border_fg {OB::Color::Type::fg};
  OB::Color _border_bg {OB::Color::Type::bg};
  std::string _line_top {"─"};
  std::string _line_bottom {"─"};
  std::string _line_left {"│"};
  std::string _line_right {"│"};
  std::string _corner_top_left {"┌"};
  std::string _corner_top_right {"┐"};
  std::string _corner_bottom_left {"└"};
  std::string _corner_bottom_right {"┘"};

  // padding
  std::size_t _padding_top {0};
  std::size_t _padding_bottom {0};
  std::size_t _padding_left {0};
  std::size_t _padding_right {0};

}; // class Rect

} // namespace OB

#endif // OB_RECT_HH
