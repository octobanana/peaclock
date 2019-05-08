#ifndef OB_READLINE_HH
#define OB_READLINE_HH

#include "ob/text.hh"

#include <cstddef>

#include <deque>
#include <string>
#include <limits>
#include <fstream>

#include <filesystem>
namespace fs = std::filesystem;

namespace OB
{

class Readline
{
public:

  Readline() = default;

  Readline& style(std::string const& style = {});
  Readline& prompt(std::string const& str, std::string const& style = {});

  std::string operator()(bool& is_running);

  void hist_push(std::string const& str);
  void hist_load(fs::path const& path);

private:

  void refresh();

  void curs_begin();
  void curs_end();
  void curs_left();
  void curs_right();

  void edit_insert(std::string const& str);
  void edit_clear();
  bool edit_delete();
  bool edit_backspace();

  void hist_prev();
  void hist_next();
  void hist_reset();
  void hist_search(std::string const& str);
  void hist_open(fs::path const& path);
  void hist_save(std::string const& str);

  std::string normalize(std::string const& str) const;

  // width and height of the terminal
  std::size_t _width {0};
  std::size_t _height {0};

  struct Style
  {
    std::string prompt;
    std::string input;
  } _style;

  struct Prompt
  {
    std::string lhs;
    std::string rhs;
    std::string fmt;
    std::string str {":"};
  } _prompt;

  struct Input
  {
    std::size_t off {0};
    std::size_t idx {0};
    std::size_t cur {0};
    std::string buf;
    OB::Text::String str;
    OB::Text::String fmt;
  } _input;

  struct History
  {
    static std::size_t constexpr npos {std::numeric_limits<std::size_t>::max()};

    struct Search
    {
      struct Result
      {
        Result(std::size_t s, std::size_t i):
          score {s},
          idx {i}
        {
        }

        std::size_t score {0};
        std::size_t idx {0};
      };

      using value_type = std::deque<Result>;

      value_type& operator()()
      {
        return val;
      }

      bool empty()
      {
        return val.empty();
      }

      void clear()
      {
        idx = History::npos;
        val.clear();
      }

      std::size_t idx {0};
      value_type val;
    } search;

    using value_type = std::deque<std::string>;

    value_type& operator()()
    {
      return val;
    }

    value_type val;
    std::size_t idx {npos};

    std::ofstream file;
  } _history;
};

} // namespace OB

#endif // OB_READLINE_HH
