#ifndef READLINE_HH
#define READLINE_HH

#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstddef>

#include <string>
#include <vector>

class Readline
{
public:

  Readline();
  ~Readline();

  Readline& prompt(std::string const& str, std::vector<std::string> const& style = {});
  std::string operator()(bool& is_running);
  void add_history(std::string const& str);

private:

  int ctrl_key(int const c) const;

  struct Prompt
  {
    std::string str {":"};
    std::vector<std::string> style;
    std::string fmt {aec::wrap(str, style)};
  } _prompt;

  struct Input
  {
    std::size_t idx {0};
    std::size_t off {0};
    std::string buf;
    std::string str;
    std::string fmt;
  } _input;

  struct History
  {
    std::vector<std::string> val;
    std::size_t idx;
  } _history;
};

#endif // READLINE_HH
