#ifndef TUI_HH
#define TUI_HH

#include "peaclock/peaclock.hh"
#include "peaclock/microphone.hh"

#include "ob/parg.hh"
#include "ob/num.hh"
#include "ob/color.hh"
#include "ob/readline.hh"
#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"
#include "ob/timer.hh"
#include "ob/belle/io.hh"
#include "ob/belle/signal.hh"

#include <boost/asio.hpp>

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

using Parg = OB::Parg;
using Tick = std::chrono::nanoseconds;
using Clock = std::chrono::steady_clock;
using Readline = OB::Readline;
using Timer = OB::Belle::asio::steady_timer;
using Read = OB::Belle::IO::Read;
using Key = OB::Belle::IO::Read::Key;
using Mouse = OB::Belle::IO::Read::Mouse;

namespace fs = std::filesystem;
namespace asio = boost::asio;
namespace Term = OB::Term;
namespace Belle = OB::Belle;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

using namespace std::chrono_literals;
using namespace std::string_literals;

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
  void winch();
  void screen_init();
  void screen_deinit();
  void await_signal();
  void render(double const delta);
  void on_tick();
  void await_tick();
  void await_read();
  bool on_read(Read::Null& ctx);
  bool on_read(Read::Mouse& ctx);
  bool on_read(Read::Key& ctx);

  void get_input();
  bool press_to_continue(std::string const& str = "ANY KEY", char32_t val = 0);

  std::optional<std::pair<bool, std::string>> command(std::string const& input);
  void command_prompt();

  void event_loop();
  int screen_size();

  void init_mic();
  int get_digitalization();

  void clear();
  void refresh();

  void draw();
  void draw_content();
  void draw_prompt_message();
  void draw_keybuf();

  void set_status(bool success, std::string const& msg);

  bool mkconfig(std::string path, bool overwrite = false);

  asio::io_context _io {1};
  Belle::Signal _sig {_io};
  Read _read {_io};

  Tick _time {0ms};
  OB::Timer<Clock> _tick_timer;
  std::chrono::time_point<Clock> _tick_begin {(Clock::time_point::min)()};
  std::chrono::time_point<Clock> _tick_end {(Clock::time_point::min)()};
  int _fps {30};
  int _fps_actual {0};
  int _fps_dropped {0};
  Tick _tick {static_cast<Tick>(1000000000 / _fps)};
  Timer _timer {_io};

  double _threshold_min {-90.0};
  double _threshold_max {0.0};
  double _digitalization_band_curr {_threshold_min};

  Microphone _mic;

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
    std::string sbuf;

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
