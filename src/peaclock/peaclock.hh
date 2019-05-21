#ifndef PEACLOCK_HH
#define PEACLOCK_HH


#include "ob/rect.hh"
using Rect = OB::Rect;

#include "ob/num.hh"
#include "ob/color.hh"
#include "ob/timer.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <ctime>
#include <cstddef>

#include <string>
#include <sstream>
#include <vector>

class Peaclock
{
public:

  void render(std::size_t const width, std::size_t const height, std::ostringstream& buf);

  struct Mode
  {
    enum Type
    {
      clock = 0,
      timer,
      stopwatch,
    };

    static Type enm(std::string const& type)
    {
      if (type.empty())
      {
        return clock;
      }

      switch (type.at(0))
      {
        case 'c':
        {
          return clock;
        }

        case 's':
        {
          return stopwatch;
        }

        case 't':
        {
          return timer;
        }

        default:
        {
          return clock;
        }
      }
    }

    static std::string str(Type type)
    {
      switch (type)
      {
        case clock:
        {
          return "clock";
        }

        case stopwatch:
        {
          return "stopwatch";
        }

        case timer:
        {
          return "timer";
        }

        default:
        {
          return {};
        }
      }
    }
  };

  struct View
  {
    enum Type
    {
      date = 0,
      ascii,
      digital,
      binary,
      icon,
    };

    static Type enm(std::string const& type)
    {
      if (type.empty())
      {
        return ascii;
      }

      switch (type.at(0))
      {
        case 'a':
        {
          return ascii;
        }

        case 'b':
        {
          return binary;
        }

        case 'd':
        {
          if (type == "date")
          {
            return date;
          }

          return digital;
        }

        case 'i':
        {
          return icon;
        }

        default:
        {
          return ascii;
        }
      }
    }

    static std::string str(Type type)
    {
      switch (type)
      {
        case date:
        {
          return "date";
        }

        case ascii:
        {
          return "ascii";
        }

        case digital:
        {
          return "digital";
        }

        case binary:
        {
          return "binary";
        }

        case icon:
        {
          return "icon";
        }

        default:
        {
          return {};
        }
      }
    }
  };

  struct Toggle
  {
    enum Type
    {
      block = 0,
      padding,
      margin,
      ratio,
      active_fg,
      active_bg,
      inactive_fg,
      inactive_bg,
      colon_fg,
      colon_bg,
      date,
      background,
    };

    static Type enm(std::string const& type)
    {
      if (type.empty())
      {
        return block;
      }

      switch (type.at(0))
      {
        case 'a':
        {
          if (type == "active-fg")
          {
            return active_fg;
          }

          return active_bg;
        }

        case 'b':
        {
          if (type == "block")
          {
            return block;
          }

          return background;
        }

        case 'c':
        {
          if (type == "colon-fg")
          {
            return colon_fg;
          }

          return colon_bg;
        }

        case 'd':
        {
          return date;
        }

        case 'i':
        {
          if (type == "inactive-fg")
          {
            return inactive_fg;
          }

          return inactive_bg;
        }

        case 'm':
        {
          return margin;
        }

        case 'p':
        {
          return padding;
        }

        case 'r':
        {
          return ratio;
        }

        default:
        {
          return block;
        }
      }
    }

    static std::string str(Type type)
    {
      switch (type)
      {
        case block:
        {
          return "block";
        }

        case padding:
        {
          return "padding";
        }

        case margin:
        {
          return "margin";
        }

        case ratio:
        {
          return "ratio";
        }

        case active_fg:
        {
          return "active-fg";
        }

        case active_bg:
        {
          return "active-bg";
        }

        case inactive_fg:
        {
          return "inactive-fg";
        }

        case inactive_bg:
        {
          return "inactive-bg";
        }

        case colon_fg:
        {
          return "colon-fg";
        }

        case colon_bg:
        {
          return "colon-bg";
        }

        case date:
        {
          return "date";
        }

        case background:
        {
          return "background";
        }

        default:
        {
          return {};
        }
      }
    }
  };

  struct Config
  {
    Mode::Type mode {Mode::clock};
    View::Type view {View::digital};
    Toggle::Type toggle {Toggle::active_bg};

    bool timer_notify {false};
    long int timer_seconds {600};
    std::string timer_exec;

    bool hour_24 {true};
    bool seconds {false};
    bool date {true};
    bool auto_size {true};
    bool auto_ratio {true};
    // bool title {true};

    OB::num_size x_block {2, 1, 64};
    OB::num_size y_block {1, 1, 64};

    OB::num_size x_ratio {2, 1, 64};
    OB::num_size y_ratio {1, 1, 64};

    OB::num_size x_border {0, 0, 64};
    OB::num_size y_border {0, 0, 64};

    OB::num_size x_space {0, 0, 64};
    OB::num_size y_space {0, 0, 64};

    OB::num_size date_padding {1, 0, 64};

    // std::size_t height_titlefmt {0};
    std::size_t height_datefmt {1};

    std::string locale {""};
    std::string timezone {""};
    // std::string titlefmt {""};
    std::string datefmt {"%a %b %d"};
    std::string fill_active {""};
    std::string fill_inactive {""};
    std::string fill_colon {""};

    struct Style
    {
      OB::Color active_fg {"", OB::Color::Type::fg};
      OB::Color inactive_fg {"", OB::Color::Type::fg};

      OB::Color active_bg {"reverse", OB::Color::Type::bg};
      OB::Color inactive_bg {"", OB::Color::Type::bg};

      OB::Color colon_fg {"", OB::Color::Type::fg};
      OB::Color colon_bg {"", OB::Color::Type::bg};

      // OB::Color title {"", OB::Color::Type::fg};
      OB::Color date {"", OB::Color::Type::fg};
      OB::Color background {"", OB::Color::Type::bg};
    } style;
  } cfg;

  // void cfg_titlefmt(std::string const& str);
  void cfg_datefmt(std::string const& str);
  bool cfg_locale(std::string const& lc);
  bool cfg_timezone(std::string const& tz);

  OB::Timer timer;
  OB::Timer stopwatch;

private:

  struct Position
  {
    enum Type
    {
      H = 0,
      h,
      M,
      m,
      S,
      s,
    };
  };

  struct Type
  {
    enum
    {
      empty = -1,
      off = 0,
      on = 1,
      newline = 2,
      colon = 3,
    };
  };

  using Clock = std::vector<int>;

  std::vector<Clock> const _symbol
  {
    // 3 x 5
    {1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1}, // 0
    {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0}, // 1
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1}, // 2
    {1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 3
    {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1}, // 4
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1}, // 5
    {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1}, // 7
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1}, // 9
  };

  Clock const _icon
  {
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,2,
    0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,2,
    0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,2,
    0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,
    0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,2,
    0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,2,
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,2,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
    0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,2,
    0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,2,
    0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,2,
    0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,0,0,1,1,1,1,0,0,0,0,0,1,1,1,0,0,0,2,
    0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,2,
  };

  Clock const _clock_digital
  {
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
  //      H   sp        h         :         M   sp        m   nl
  };

  Clock const _clock_digital_seconds
  {
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  0, 3, 0,  0, 0, 0,  0,  0, 0, 0,  2,
    0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0,  0, 0, 0,  2,
  //      H   sp        h         :         M   sp        m         :         S   sp        s   nl
  };

  Clock const _clock_binary
  {
    -1, 0, -1, 0,  2,
    -1, 0,  0, 0,  2,
     0, 0,  0, 0,  2,
     0, 0,  0, 0,  2,
  // H  h   M  m   nl
  };

  Clock const _clock_binary_seconds
  {
    -1, 0, -1, 0, -1, 0,  2,
    -1, 0,  0, 0,  0, 0,  2,
     0, 0,  0, 0,  0, 0,  2,
     0, 0,  0, 0,  0, 0,  2,
  // H  h   M  m   S  s   nl
  };

  struct Ctx
  {
    Clock value
    {
       0, 0, 0, 0, 0, 0
    // H  h  M  m  S  s
    };

    Clock buffer;

    Rect text;
    Rect block;
    // Rect background;

    std::string datefmt;

    std::size_t x {0};
    std::size_t y {0};

    std::size_t width {0};
    std::size_t height {0};

    std::size_t x_blocks {0};
    std::size_t y_blocks {0};

    std::size_t x_spaces {0};
    std::size_t y_spaces {0};

    std::size_t x_block {0};
    std::size_t y_block {0};

    std::size_t x_begin {0};
    std::size_t y_begin {0};
  } _ctx;

  std::size_t const npos {std::numeric_limits<std::size_t>::max()};

  std::size_t find(Clock const& vec, int const val, std::size_t const pos = 0) const;
  std::size_t count(Clock const& vec, int const val) const;
  std::size_t count_x_blocks(Clock const& clock) const;
  std::size_t count_y_blocks(Clock const& clock) const;
  void extract_digits(int const num, int& t0, int& t1) const;

  void init_ctx(std::size_t const width, std::size_t const height);

  void calc_xy_block();
  void calc_xy_ratio();
  void calc_xy_begin();

  void fill_digital(std::size_t width, std::size_t begin, std::size_t end, Position::Type type);
  void fill_binary(std::size_t width, std::size_t col, int num);

  void set_clock_value();
  void set_date(std::tm const& time_now);
  void set_clock_digital();
  void set_clock_binary();

  // void draw_background(std::size_t const width, std::size_t const height, std::ostringstream& buf);
  // void draw_title(std::ostringstream& buf);
  void draw_clock(std::ostringstream& buf);
  void draw_ascii(std::ostringstream& buf);
  void draw_date(std::ostringstream& buf);
};

#endif // PEACLOCK_HH
