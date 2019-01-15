#ifndef OB_TERM_HH
#define OB_TERM_HH

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <cstdio>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <iomanip>
#include <streambuf>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <vector>
#include <regex>
#include <chrono>
#include <thread>
#include <utility>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace OB::Term
{

class Mode
{
public:

  Mode()
  {
  }

  ~Mode()
  {
    if (! _cooked)
    {
      set_cooked();
    }
  }

  void set_cooked()
  {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &_old) == -1)
    {
      throw std::runtime_error("tcsetattr failed");
    }

    _cooked = true;
  }

  void set_raw()
  {
    if (tcgetattr(STDIN_FILENO, &_old) == -1)
    {
      throw std::runtime_error("tcgetattr failed");
    }

    _raw = _old;
    // _raw.c_iflag &= static_cast<tcflag_t>(~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON));
    // _raw.c_oflag &= static_cast<tcflag_t>(~(OPOST));
    _raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ECHONL | ICANON | ISIG | IEXTEN));
    _raw.c_cflag &= static_cast<tcflag_t>(~(CSIZE | PARENB));
    _raw.c_cflag |= static_cast<tcflag_t>(CS8);
    _raw.c_cc[VMIN]  = 0;
    _raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &_raw) == -1)
    {
      throw std::runtime_error("tcsetattr failed");
    }

    _cooked = false;
  }

private:

  termios _old;
  termios _raw;
  bool _cooked {true};
}; // class Mode

inline std::string env_var(std::string const& str)
{
  std::string res;

  if (char const* envar = std::getenv(str.data()))
  {
    res = envar;
  }

  return res;
}

inline bool is_term(std::size_t fd_)
{
  switch (fd_)
  {
    case STDIN_FILENO: return isatty(STDIN_FILENO);
    case STDOUT_FILENO: return isatty(STDOUT_FILENO);
    case STDERR_FILENO: return isatty(STDERR_FILENO);
    default: return false;
  }
}

inline bool is_colorterm()
{
  auto const colorterm = env_var("COLORTERM");

  if (colorterm == "truecolor" || colorterm == "24bit")
  {
    return true;
  }

  return false;
}

inline int width(std::size_t& width_, std::size_t fd_ = STDOUT_FILENO)
{
  if (fd_ != STDIN_FILENO && fd_ != STDOUT_FILENO && fd_ != STDERR_FILENO)
  {
    return -1;
  }

  winsize w;
  ioctl(fd_, TIOCGWINSZ, &w);

  width_ = w.ws_col;

  return 0;
}

inline int height(std::size_t& height_, std::size_t fd_ = STDOUT_FILENO)
{
  if (fd_ != STDIN_FILENO && fd_ != STDOUT_FILENO && fd_ != STDERR_FILENO)
  {
    return -1;
  }

  winsize w;
  ioctl(fd_, TIOCGWINSZ, &w);

  height_ = w.ws_row;

  return 0;
}

inline int size(std::size_t& width_, std::size_t& height_, std::size_t fd_ = STDOUT_FILENO)
{
  if (fd_ != STDIN_FILENO && fd_ != STDOUT_FILENO && fd_ != STDERR_FILENO)
  {
    return -1;
  }

  winsize w;
  ioctl(fd_, TIOCGWINSZ, &w);

  width_ = w.ws_col;
  height_ = w.ws_row;

  return 0;
}

namespace ANSI_Escape_Codes
{

// standard escaped characters
std::string const nl {"\n"};
std::string const cr {"\r"};
std::string const tab {"\t"};
std::string const alert {"\a"};
std::string const backspace {"\b"};
std::string const backslash {"\\"};

// escape code sequence
std::string const esc {"\x1b"};

// clears all attributes
std::string const reset {esc + "[0m"};

// style
std::string const bold {esc + "[1m"};
std::string const dim {esc + "[2m"};
std::string const italic {esc + "[3m"};
std::string const underline {esc + "[4m"};
std::string const blink {esc + "[5m"};
std::string const rblink {esc + "[6m"};
std::string const reverse {esc + "[7m"};
std::string const conceal {esc + "[8m"};
std::string const cross {esc + "[9m"};

// erasing
std::string const clear {esc + "c"};
std::string const erase_end {esc + "[K"};
std::string const erase_start {esc + "[1K"};
std::string const erase_line {esc + "[2K"};
std::string const erase_down {esc + "[J"};
std::string const erase_up {esc + "[1J"};
std::string const erase_screen {esc + "[2J"};

// cursor visibility
std::string const cursor_hide {esc + "[?25l"};
std::string const cursor_show {esc + "[?25h"};

// cursor movement
std::string const cursor_home {esc + "[H"};
std::string const cursor_up {esc + "[1A"};
std::string const cursor_down {esc + "[1B"};
std::string const cursor_right {esc + "[1C"};
std::string const cursor_left {esc + "[1D"};

// cursor position
std::string const cursor_save {esc + "7"};
std::string const cursor_load {esc + "8"};

// foreground color
std::string const fg_black {esc + "[30m"};
std::string const fg_red {esc + "[31m"};
std::string const fg_green {esc + "[32m"};
std::string const fg_yellow {esc + "[33m"};
std::string const fg_blue {esc + "[34m"};
std::string const fg_magenta {esc + "[35m"};
std::string const fg_cyan {esc + "[36m"};
std::string const fg_white {esc + "[37m"};

// background color
std::string const bg_black {esc + "[40m"};
std::string const bg_red {esc + "[41m"};
std::string const bg_green {esc + "[42m"};
std::string const bg_yellow {esc + "[43m"};
std::string const bg_blue {esc + "[44m"};
std::string const bg_magenta {esc + "[45m"};
std::string const bg_cyan {esc + "[46m"};
std::string const bg_white {esc + "[47m"};

inline std::string fg_256(std::string const& str_)
{
  auto const n = std::stoi(str_);
  if (n < 0 || n > 256) return {};
  std::stringstream ss;
  ss << esc << "[38;5;" << str_ << "m";

  return ss.str();
}

inline std::string bg_256(std::string const& str_)
{
  auto const n = std::stoi(str_);
  if (n < 0 || n > 256) return {};
  std::stringstream ss;
  ss << esc << "[48;5;" << str_ << "m";

  return ss.str();
}

inline std::string htoi(std::string const& str_)
{
  std::stringstream ss;
  ss << str_;
  unsigned int n;
  ss >> std::hex >> n;

  return std::to_string(n);
}

inline bool valid_hstr(std::string& str_)
{
  std::smatch m;
  std::regex rx {"^#?((?:[0-9a-fA-F]{3}){1,2})$"};

  if (std::regex_match(str_, m, rx, std::regex_constants::match_not_null))
  {
    std::string hstr {m[1]};

    if (hstr.size() == 3)
    {
      std::stringstream ss;
      ss << hstr[0] << hstr[0] << hstr[1] << hstr[1] << hstr[2] << hstr[2];
      hstr = ss.str();
    }

    str_ = hstr;

    return true;
  }

  return false;
}

inline std::string fg_true(std::string str_)
{
  if (! valid_hstr(str_))
  {
    return {};
  }

  std::string const h1 {str_.substr(0, 2)};
  std::string const h2 {str_.substr(2, 2)};
  std::string const h3 {str_.substr(4, 2)};

  std::stringstream ss; ss
  << esc << "[38;2;"
  << htoi(h1) << ";"
  << htoi(h2) << ";"
  << htoi(h3) << "m";

  return ss.str();
}

inline std::string bg_true(std::string str_)
{
  if (! valid_hstr(str_))
  {
    return {};
  }

  std::string const h1 {str_.substr(0, 2)};
  std::string const h2 {str_.substr(2, 2)};
  std::string const h3 {str_.substr(4, 2)};

  std::stringstream ss; ss
  << esc << "[48;2;"
  << htoi(h1) << ";"
  << htoi(h2) << ";"
  << htoi(h3) << "m";

  return ss.str();
}

inline std::string cursor_set(std::size_t x_, std::size_t y_)
{
  std::stringstream ss;
  ss << esc << "[" << y_ << ";" << x_ << "H";

  return ss.str();
}

inline int cursor_get(std::size_t& x_, std::size_t& y_, bool mode_ = true)
{
  Term::Mode mode;

  if (mode_)
  {
    mode.set_raw();
  }

  std::cout << (esc + "[6n") << std::flush;

  char buf[32];
  std::uint8_t i {0};

  // attempt to read response for up to 1 second
  for (int retry = 20; i < sizeof(buf) - 1; ++i)
  {
    while (retry-- > 0 && read(STDIN_FILENO, &buf[i], 1) != 1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (buf[i] == 'R')
    {
      break;
    }
  }

  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[')
  {
    return -1;
  }

  int x;
  int y;
  if (std::sscanf(&buf[2], "%d;%d", &y, &x) != 2)
  {
    return -1;
  }

  x_ = static_cast<std::size_t>(x);
  y_ = static_cast<std::size_t>(y);

  return 0;
}

template<typename T>
std::string wrap(T const val_, std::string const attr_, bool color_ = true)
{
  std::stringstream ss;

  if (color_)
  {
    ss
    << attr_
    << val_
    << reset;
  }
  else
  {
    ss << val_;
  }

  return ss.str();
}

template<typename T>
std::string wrap(T const val_, std::vector<std::string> const attr_, bool color_ = true)
{
  std::stringstream ss;

  if (color_)
  {
    for (auto const& e : attr_)
    {
      ss << e;
    }
    ss << val_ << reset;
  }
  else
  {
    ss << val_ << reset;
  }

  return ss.str();
}

} // namespace ANSI_Escape_Codes

class ostream : public std::ostream
{
protected:

  class streambuf : public std::streambuf
  {
    using string = std::basic_string<char_type>;

  public:

    explicit streambuf(std::streambuf* streambuf_,
      std::size_t width_ = std::numeric_limits<std::size_t>::max()) :
      _streambuf {streambuf_},
      _width {width_}
    {
    }

    ~streambuf()
    {
      flush();
    }

    streambuf& flush()
    {
      if (! _buffer.empty())
      {
        if (_size)
        {
          if (! _white_space && ! _buffer.empty() && _buffer.back() == ' ')
          {
            _buffer.erase(_buffer.size() - 1);
          }

          _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
          _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));
        }
        else
        {
          _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));
        }

        _streambuf->sputc('\n');
      }

      _size = 0;
      _buffer.clear();
      _esc_seq.clear();

      return *this;
    }

    streambuf& push(std::size_t val_)
    {
      flush();

      _level += val_;

      _prefix = string(_level * _indent, ' ');

      return *this;
    }

    streambuf& pop(std::size_t val_)
    {
      flush();

      if (_level > val_)
      {
        _level -= val_;
      }
      else
      {
        _level = 0;
      }

      _prefix = string(_level * _indent, ' ');

      return *this;
    }

    streambuf& width(std::size_t val_)
    {
      _width = val_;

      return *this;
    }

    streambuf& escape_codes(bool val_)
    {
      _escape_codes = val_;
      _esc_seq.clear();

      return *this;
    }

    streambuf& line_wrap(bool val_)
    {
      _line_wrap = val_;

      return *this;
    }

    streambuf& first_wrap(bool val_)
    {
      _first_wrap = val_;

      return *this;
    }

    streambuf& auto_wrap(bool val_)
    {
      _auto_wrap = val_;
      _prefix.clear();

      return *this;
    }

    streambuf& word_break(bool val_)
    {
      _word_break = val_;

      return *this;
    }

    streambuf& white_space(bool val_)
    {
      _white_space = val_;

      return *this;
    }

    streambuf& indent(std::size_t val_)
    {
      _indent = val_;
      _prefix = string(_level * _indent, ' ');

      return *this;
    }

    streambuf& level(std::size_t val_)
    {
      _level = val_;
      _prefix = string(_level * _indent, ' ');

      return *this;
    }

  protected:

    int_type overflow(int_type ch_)
    {
      if (traits_type::eq_int_type(traits_type::eof(), ch_))
      {
        return traits_type::not_eof(ch_);
      }

      // handle auto wrap prefix
      if (ch_ != ' ' && ch_ != '\t')
      {
        _is_prefix = false;
      }

      // handle ansi escape codes
      if (! _esc_seq.empty())
      {
        if (_esc_seq.size() == 1)
        {
          if (ch_ != '[' && ch_ != '(' && ch_ != ')' && ch_ != '#')
          {
            _esc_seq += ch_;
            if (_escape_codes)
            {
              _buffer += _esc_seq;
            }
            _esc_seq.clear();
          }
          else if (ch_ == '[' || ch_ == '(' || ch_ == ')' || ch_ == '#')
          {
            _esc_seq += ch_;
          }
          else if (ch_ == std::isalpha(static_cast<unsigned char>(ch_)))
          {
            _esc_seq += ch_;
            if (_escape_codes)
            {
              _buffer += _esc_seq;
            }
            _esc_seq.clear();
          }
          else
          {
            _esc_seq.clear();
          }
        }
        else if (_esc_seq.size() == 2 && _esc_seq.back() == '#')
        {
          _esc_seq += ch_;
          if (_escape_codes)
          {
            _buffer += _esc_seq;
          }
          _esc_seq.clear();
        }
        else if (std::isalpha(static_cast<unsigned char>(ch_)))
        {
          _esc_seq += ch_;
          if (_escape_codes)
          {
            _buffer += _esc_seq;
          }
          _esc_seq.clear();
        }
        else
        {
          _esc_seq += ch_;
        }

        return ch_;
      }

      switch (ch_)
      {
        case '\t':
        {
          if (! _first_wrap && _level == 0)
          {
            // don't wrap first line when level is 0
            // block left intentionally empty
          }
          else if (_line_wrap && (_size + _indent >= _width - _prefix.size()))
          {
            if (auto pos = _buffer.find_last_of(" ");
              _word_break && pos != string::npos)
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(pos));

              _size = _buffer.size() - pos - 1;
              _buffer = _buffer.substr(pos + 1);
            }
            else
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));

              _size = 0;
              _buffer.clear();
            }

            _streambuf->sputc('\n');
          }

          if (_auto_wrap && _is_prefix)
          {
            _level = 1;
            _prefix += string(_indent, ' ');
          }
          else if (_white_space || _buffer.empty())
          {
            _size += _indent;
            _buffer += string(_indent, ' ');
          }
          else if (! _buffer.empty() && _buffer.back() != ' ')
          {
            _size += _indent;
            _buffer += string(_indent, ' ');
          }

          return ch_;
        }

        case '\x1b':
        {
          _esc_seq.clear();
          _esc_seq += ch_;

          return ch_;
        }

        case '\a':
        {
          _buffer += ch_;

          return ch_;
        }

        case '\b':
        {
          --_size;
          _buffer += ch_;

          return ch_;
        }

        case '\n':
        case '\r':
        {
          if (! _white_space && ! _buffer.empty() && _buffer.back() == ' ')
          {
            _buffer.erase(_buffer.size() - 1);
          }

          _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
          _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));
          _streambuf->sputc(ch_);

          _size = 0;
          _buffer.clear();

          if (_auto_wrap)
          {
            _level = 0;
            _is_prefix = true;
            _prefix.clear();
          }

          return ch_;
        }

        case ' ':
        {
          if (! _first_wrap && _level == 0)
          {
            // don't wrap first line when level is 0
            // block left intentionally empty
          }
          else if (_line_wrap && (_size + 1 + _prefix.size() >= _width))
          {
            ++_size;
            _buffer += " ";

            if (auto pos = _buffer.find_last_of(" ");
                _word_break && pos != string::npos)
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(pos));

              _size = _buffer.size() - pos - 1;
              _buffer = _buffer.substr(pos + 1);
            }
            else
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));

              _size = 0;
              _buffer.clear();
            }

            _streambuf->sputc('\n');

            return ch_;
          }

          if (_auto_wrap && _is_prefix)
          {
            _level = 1;
            _prefix += " ";
          }
          else if (_white_space)
          {
            ++_size;
            _buffer += " ";
          }
          else if (! _buffer.empty() && _buffer.back() != ' ')
          {
            ++_size;
            _buffer += " ";
          }

          return ch_;
        }

        default:
        {
          if (! _first_wrap && _level == 0)
          {
            // don't wrap first line when level is 0
            // block left intentionally empty
          }
          else if (_line_wrap && (_size + _prefix.size() >= _width))
          {
            if (auto pos = _buffer.find_last_of(" ");
                _word_break && pos != string::npos)
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(pos));

              _size = _buffer.size() - pos - 1;
              _buffer = _buffer.substr(pos + 1);
            }
            else
            {
              _streambuf->sputn(_prefix.data(), static_cast<std::streamsize>(_prefix.size()));
              _streambuf->sputn(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));

              _size = 0;
              _buffer.clear();
            }

            _streambuf->sputc('\n');
          }

          ++_size;
          _buffer += ch_;

          return ch_;
        }
      }
    }

    // output streambuf
    std::streambuf* _streambuf;

    // maximum output width
    std::size_t _width;

    // number of spaces to indent
    std::size_t _indent {2};

    // indentation level
    std::size_t _level {0};

    // stream should wrap around at output width
    bool _line_wrap {true};

    // stream should wrap around at output width when level=0
    bool _first_wrap {true};

    // auto calc indent when wrapping line
    bool _auto_wrap {false};

    // stream should break words on wrap
    bool _word_break {true};

    // stream should preserve whitespace
    bool _white_space {true};

    // stream should output escape codes
    bool _escape_codes {true};

    // used to calc the indent for auto wrap
    bool _is_prefix {true};

    // size of buffer minus special chars
    std::size_t _size {0};

    // prefix string prepended onto start of a new line
    string _prefix {""};

    // buffer string for temporary storage of chars
    string _buffer {""};

    // buffer string for escape sequences
    string _esc_seq {""};
  }; // class streambuf

public:

  explicit ostream(std::ostream& os_, std::size_t indent_width_ = 2,
    std::size_t output_width_ = std::numeric_limits<std::size_t>::max()) :
    std::ostream {&_stream},
    _stream {os_.rdbuf(), output_width_}
  {
    indent(indent_width_);
  }

  ostream& flush()
  {
    _stream.flush();
    std::ostream::flush();

    return *this;
  }

  ostream& push(std::size_t val_ = 1)
  {
    _stream.push(val_);
    std::ostream::flush();

    return *this;
  }

  ostream& pop(std::size_t val_ = 1)
  {
    _stream.pop(val_);
    std::ostream::flush();

    return *this;
  }

  ostream& width(std::size_t val_ = std::numeric_limits<std::size_t>::max())
  {
    _stream.width(val_);

    return *this;
  }

  ostream& escape_codes(bool val_ = true)
  {
    _stream.escape_codes(val_);

    return *this;
  }

  ostream& line_wrap(bool val_ = true)
  {
    _stream.line_wrap(val_);

    return *this;
  }

  ostream& first_wrap(bool val_ = true)
  {
    _stream.first_wrap(val_);

    return *this;
  }

  ostream& auto_wrap(bool val_ = true)
  {
    _stream.auto_wrap(val_);

    return *this;
  }

  ostream& word_break(bool val_ = true)
  {
    _stream.word_break(val_);

    return *this;
  }

  ostream& white_space(bool val_ = true)
  {
    _stream.white_space(val_);

    return *this;
  }

  ostream& indent(std::size_t val_ = 2)
  {
    _stream.indent(val_);

    return *this;
  }

  ostream& level(std::size_t val_ = 0)
  {
    _stream.level(val_);

    return *this;
  }

protected:

  streambuf _stream;
}; // class ostream

namespace iomanip
{

class flush
{
public:

  friend Term::ostream& operator<<(Term::ostream& os_, flush const&)
  {
    os_.flush();

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, flush const&)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.flush();

    return derived;
  }
}; // class flush

class push
{
public:

  explicit push(std::size_t val_ = 1) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, push const& obj_)
  {
    os_.push(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, push const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.push(obj_._val);

    return derived;
  }

private:

  std::size_t _val;
}; // class push

class pop
{
public:

  explicit pop(std::size_t val_ = 1) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, pop const& obj_)
  {
    os_.pop(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, pop const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.pop(obj_._val);

    return derived;
  }

private:

  std::size_t _val;
}; // class pop

class line_wrap
{
public:

  explicit line_wrap(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, line_wrap const& obj_)
  {
    os_.line_wrap(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, line_wrap const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.line_wrap(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class line_wrap

class first_wrap
{
public:

  explicit first_wrap(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, first_wrap const& obj_)
  {
    os_.first_wrap(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, first_wrap const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.first_wrap(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class first_wrap

class word_break
{
public:

  explicit word_break(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, word_break const& obj_)
  {
    os_.word_break(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, word_break const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.word_break(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class word_break

class white_space
{
public:

  explicit white_space(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, white_space const& obj_)
  {
    os_.white_space(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, white_space const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.white_space(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class white_space

class escape_codes
{
public:

  explicit escape_codes(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, escape_codes const& obj_)
  {
    os_.escape_codes(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, escape_codes const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.escape_codes(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class escape_codes

class width
{
public:

  explicit width(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, width const& obj_)
  {
    os_.width(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, width const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.width(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class width

class indent
{
public:

  explicit indent(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, indent const& obj_)
  {
    os_.indent(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, indent const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.indent(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class indent

class level
{
public:

  explicit level(bool val_ = true) :
    _val {val_}
  {
  }

  friend Term::ostream& operator<<(Term::ostream& os_, level const& obj_)
  {
    os_.level(obj_._val);

    return os_;
  }

  friend Term::ostream& operator<<(std::ostream& os_, level const& obj_)
  {
    auto& derived = dynamic_cast<Term::ostream&>(os_);
    derived.level(obj_._val);

    return derived;
  }

private:

  bool _val;
}; // class level

} // namespace iomanip

} // namespace OB::Term

#endif // OB_TERM_HH
