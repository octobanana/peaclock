#ifndef TUI_HH
#define TUI_HH

#include "peaclock/peaclock.hh"

#include "ob/parg.hh"
using Parg = OB::Parg;

#include "ob/num.hh"
#include "ob/color.hh"
#include "ob/readline.hh"
#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <string>
#include <vector>
#include <sstream>
#include <utility>
#include <optional>

#include <filesystem>
namespace fs = std::filesystem;

class Tui
{
public:

  Tui(Parg const& parg);

  Tui& init(fs::path const& path = {});
  void base_config(fs::path const& path);
  void load_config(fs::path const& path);
  void load_hist_command(fs::path const& path);
  void run();

private:

  void get_input();
  bool press_to_continue(std::string const& str = "ANY KEY", char32_t val = 0);

  std::optional<std::pair<bool, std::string>> command(std::string const& input);
  void command_prompt();

  void event_loop();
  int screen_size();

  void clear();
  void refresh();

  void draw();
  void draw_content();
  void draw_prompt_message();
  void draw_keybuf();

  void set_status(bool success, std::string const& msg);

  bool mkconfig(std::string path, bool overwrite = false);

  Parg const& _pg;
  bool const _colorterm;
  OB::Term::Mode _term_mode;
  OB::Readline _readline;
  Peaclock _peaclock;

  struct Ctx
  {
    // base config directory
    fs::path base_config;

    // current terminal size
    std::size_t width {0};
    std::size_t height {0};

    // minimum terminal size
    std::size_t width_min {4};
    std::size_t height_min {2};

    // output buffer
    std::ostringstream buf;

    // control when to exit the event loop
    bool is_running {true};

    // interval between reading a keypress
    OB::num input_interval {50, 10, 1000};

    // total time spent in input loop
    OB::num refresh_rate {1000, 10, 60000};

    // input key buffers
    OB::Text::Char32 key;
    std::vector<OB::Text::Char32> keys;

    // command prompt
    struct Prompt
    {
      std::string str;
      int count {0};
      int timeout {0};
      OB::num rate {5000, 0, 60000};
    } prompt;

    struct Style
    {
      OB::Color text {"", OB::Color::Type::fg};
      OB::Color prompt {"", OB::Color::Type::fg};
      OB::Color success {"green", OB::Color::Type::fg};
      OB::Color error {"red", OB::Color::Type::fg};

      OB::Color background {"", OB::Color::Type::bg};

      // stores success or error color for status output
      OB::Color prompt_status {"", OB::Color::Type::fg};
    } style;

    struct Regex
    {
      std::string const str_s {"(?:'[^'\\\\]*(?:\\\\.[^'\\\\]*)*')"};
      std::string const str_d {"(?:\"[^\\\"\\\\]*(?:\\\\.[^\\\"\\\\]*)*\")"};
      std::string const str {"(?:" + str_s + "|" + str_d + ")"};
    } rx;
  } _ctx;
};

#endif // TUI_HH
