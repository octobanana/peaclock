/*
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Belle IO
0.6.0 develop
https://octobanana.com/software/belle

Licensed under the MIT License
Copyright (c) 2018-2019 Brett Robinson <https://octobanana.com/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef OB_BELLE_IO_HH
#define OB_BELLE_IO_HH

#include "ob/belle/config.hh"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <csignal>

#include <regex>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <iterator>
#include <optional>
#include <algorithm>
#include <functional>

namespace OB::Belle {

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
namespace IO {

// aliases
namespace asio = boost::asio;
using error_code = boost::system::error_code;

class Read final {
public:

  struct Null {
    std::string str;
    char32_t ch {0};
  };

  struct Key {
    enum : char32_t {
      Bell = 7,
      Tab = 9,
      Newline = 10,
      Enter = 13,
      Escape = 27,
      Space = 32,
      Backspace = 127,
      Up = 0xF0000,
      Down,
      Left,
      Right,
      Home,
      End,
      Delete,
      Insert,
      Page_up,
      Page_down
    };
    inline static const std::unordered_map<std::string, char32_t> map {
      {"bell", Bell},
      {"tab", Tab},
      {"newline", Newline},
      {"enter", Enter},
      {"escape", Escape},
      {"space", Space},
      {"backspace", Backspace},
      {"up", Up},
      {"down", Down},
      {"left", Left},
      {"right", Right},
      {"home", Home},
      {"end", End},
      {"delete", Delete},
      {"insert", Insert},
      {"pageup", Page_up},
      {"pagedown", Page_down},
    };
    std::string str;
    char32_t ch {0};
  };

  struct Mouse {
    enum : char32_t {
      Press_left = 0xF1000,
      Press_middle,
      Press_right,
      Release_left,
      Release_middle,
      Release_right,
      Release,
      Scroll_up,
      Scroll_down
    };
    // enum Action {
    //   Press = 0,
    //   Release,
    //   Scroll
    // };
    // enum Direction {
    //   up = 0,
    //   down,
    // };
    // enum Btn {
    //   left = 0,
    //   middle,
    //   right
    // };
    struct Pos {
      std::size_t x {0};
      std::size_t y {0};
    };
    std::string str;
    char32_t ch {0};
    Pos pos;
  };

  using Ctx = std::variant<Null, Key, Mouse>;
  using fn_on_read = std::function<void(Ctx const&)>;

  explicit Read(asio::io_context& io_) noexcept : _io {io_} {}

  void run() {
    asio::spawn(_io, [&](auto yield) {
      for (;;) {
        read_impl(yield);
      }
    });
  }

  void cancel() {
    _istream.cancel();
  }

  Read& on_read(fn_on_read v) {
    _on_read = v;
    return *this;
  }

private:

  std::optional<std::vector<std::string>> rx_match(std::string const& str, std::regex rx) {
    std::smatch m;
    if (std::regex_match(str, m, rx, std::regex_constants::match_not_null)) {
      std::vector<std::string> v;
      for (auto const& e : m) {
        v.emplace_back(std::string(e));
      }
      return v;
    }
    return {};
  }

  char32_t utf8_to_char32(std::string_view str) {
    if (str.empty()) {
      return 0;
    }
    if ((str.at(0) & 0x80) == 0) {
      return static_cast<char32_t>(str.at(0));
    }
    else if ((str.at(0) & 0xE0) == 0xC0 && str.size() == 2) {
      return (static_cast<char32_t>(str[0] & 0x1F) << 6) |
        static_cast<char32_t>(str[1] & 0x3F);
    }
    else if ((str.at(0) & 0xF0) == 0xE0 && str.size() == 3) {
      return (static_cast<char32_t>(str[0] & 0x0F) << 12) |
        (static_cast<char32_t>(str[1] & 0x3F) << 6) |
        static_cast<char32_t>(str[2] & 0x3F);
    }
    else if ((str.at(0) & 0xF8) == 0xF0 && str.size() == 4) {
      return (static_cast<char32_t>(str[0] & 0x07) << 18) |
        (static_cast<char32_t>(str[1] & 0x3F) << 12) |
        (static_cast<char32_t>(str[2] & 0x3F) << 6) |
        static_cast<char32_t>(str[3] & 0x3F);
    }
    return 0;
  }

  void read_impl(asio::yield_context yield) {
    try {
      _istream.async_wait(asio::posix::stream_descriptor::wait_read, yield);
      _buf_size += _istream.async_read_some(asio::buffer(&_buf[0] + _pos_write, _buf_max - _pos_write), yield);

      std::size_t _pos_read {0};
      while (_pos_read < _buf_size) {
        std::size_t bytes = [c = _buf[_pos_read]]() {
          if ((c & 0x80) == 0x00) {return 1;}
          if ((c & 0xe0) == 0xc0) {return 2;}
          if ((c & 0xf0) == 0xe0) {return 3;}
          if ((c & 0xf8) == 0xf0) {return 4;}
          throw std::runtime_error("invalid utf-8 codepoint");
        }();

        if (bytes > 1) {
          if (_pos_read + bytes - 1 >= _buf_size) {
            goto read_more;
          }
          _ctx = Key{{&_buf[_pos_read], bytes}, utf8_to_char32({&_buf[_pos_read], bytes})};
          _pos_read += bytes;
        }
        else {
          if (_buf[_pos_read] != 0x1b) {
            _ctx = Key{{&_buf[_pos_read], 1}, static_cast<char32_t>(_buf[_pos_read])};
            _pos_read += 1;
          }
          else {
            if (_pos_read + 1 >= _buf_size) {
              asio::high_resolution_timer timer {_io};
              timer.expires_from_now(_interval);
              timer.async_wait([&](auto ec) {
                if (ec) {return;}
                _istream.cancel();
              });

              try {
                _istream.async_wait(asio::posix::stream_descriptor::wait_read, yield);
                timer.cancel();
                goto read_more;
              }
              catch (...) {
                // escape
                _ctx = Key{{&_buf[_pos_read], 1}, static_cast<char32_t>(_buf[_pos_read])};
                _pos_read += 1;
              }
            }
            else if (_buf[_pos_read + 1] == '[') {
              // ctrlseq
              if (_pos_read + 2 >= _buf_size) {
                goto read_more;
              }
              // std::cerr << "read> " << _buf[_pos_read + 2] << "\n";

              if (_buf[_pos_read + 2] >= '0' && _buf[_pos_read + 2] <= '9') {
                if (_pos_read + 3 >= _buf_size) {
                  goto read_more;
                }
                if (_buf[_pos_read + 3] == '~') {
                  switch (_buf[_pos_read + 2]) {
                    case '1': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::Home};
                      break;
                    }
                    case '2': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::Insert};
                      break;
                    }
                    case '3': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::Delete};
                      break;
                    }
                    case '4': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::End};
                      break;
                    }
                    case '5': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::Page_up};
                      break;
                    }
                    case '6': {
                      _ctx = Key{{&_buf[_pos_read], 4}, Key::Page_down};
                      break;
                    }
                    default: {
                      _ctx = Null{{&_buf[_pos_read], 4}};
                      break;
                    }
                  }
                  _pos_read += 4;
                }
              }
              else
              {
                switch (_buf[_pos_read + 2]) {
                  // case 'I': {
                  //   // TODO add Event Ctx variant type
                  //   _ctx = Null{{&_buf[_pos_read], 4}};
                  //   _pos_read += 4;
                  //   break;
                  // }
                  // case 'O': {
                  //   // TODO add Event Ctx variant type
                  //   _ctx = Null{{&_buf[_pos_read], 4}};
                  //   _pos_read += 4;
                  //   break;
                  // }
                  case 'A': {
                    _ctx = Key{{&_buf[_pos_read], 3}, Key::Up};
                    _pos_read += 3;
                    break;
                  }
                  case 'B': {
                    _ctx = Key{{&_buf[_pos_read], 3}, Key::Down};
                    _pos_read += 3;
                    break;
                  }
                  case 'C': {
                    _ctx = Key{{&_buf[_pos_read], 3}, Key::Right};
                    _pos_read += 3;
                    break;
                  }
                  case 'D': {
                    _ctx = Key{{&_buf[_pos_read], 3}, Key::Left};
                    _pos_read += 3;
                    break;
                  }
                  case '<': {
                    // mouse 1000;1006
                    bool full {false};
                    std::size_t idx {_pos_read + 3};
                    for (; idx < _buf_size; ++idx)
                    {
                      if (_buf[idx] >= 0x40 && _buf[idx] <= 0x7E) {
                        full = true;
                        break;
                      }
                    }
                    if (!full) {
                      goto read_more;
                    }

                    _ctx = Mouse{{&_buf[_pos_read], idx - _pos_read + 1}};
                    auto& m = std::get<Mouse>(_ctx);
                    auto const match = rx_match(m.str.substr(3), std::regex("([0126]{1})([45]{1})?;([0-9]+);([0-9]+)([mM]{1})"));
                    if (!match) {
                      _ctx = Null{{&_buf[_pos_read], idx - _pos_read + 1}};
                    }
                    else {
                      m.pos.x = std::stoul(match->at(3));
                      m.pos.y = std::stoul(match->at(4));
                      auto btn = std::stoul(match->at(1));
                      if (match->at(5) == "M") {
                        switch (btn) {
                          case 0: {
                            m.ch = Mouse::Press_left;
                            break;
                          }
                          case 1: {
                            m.ch = Mouse::Press_middle;
                            break;
                          }
                          case 2: {
                            m.ch = Mouse::Press_right;
                            break;
                          }
                          case 6: {
                            switch (std::stoul(match->at(2))) {
                              case 4: {
                                m.ch = Mouse::Scroll_up;
                                break;
                              }
                              case 5: {
                                m.ch = Mouse::Scroll_down;
                                break;
                              }
                            }
                            break;
                          }
                        }
                      }
                      else {
                        switch (btn) {
                          case 0: {
                            m.ch = Mouse::Release_left;
                            break;
                          }
                          case 1: {
                            m.ch = Mouse::Release_middle;
                            break;
                          }
                          case 2: {
                            m.ch = Mouse::Release_right;
                            break;
                          }
                        }
                      }
                    }
                    _pos_read += idx - _pos_read + 1;
                    break;
                  }
                  case 'M': {
                    // mouse 1000
                    if (_pos_read + 5 > _buf_size) {
                      goto read_more;
                    }
                    _ctx = Mouse{{&_buf[_pos_read], 5}};
                    auto& m = std::get<Mouse>(_ctx);
                    m.pos.x = std::stoul(std::string(_buf[_pos_read + 4], 1));
                    m.pos.y = std::stoul(std::string(_buf[_pos_read + 5], 1));
                    switch (_buf[_pos_read + 3] & 0x03) {
                      case 0: {
                        if (_buf[_pos_read + 3] & 0x40) {
                          m.ch = Mouse::Scroll_up;
                        }
                        else {
                          m.ch = Mouse::Press_left;
                        }
                        break;
                      }
                      case 1: {
                        if (_buf[_pos_read + 3] & 0x40) {
                          m.ch = Mouse::Scroll_down;
                        }
                        else {
                          m.ch = Mouse::Press_middle;
                        }
                        break;
                      }
                      case 2: {
                        m.ch = Mouse::Press_right;
                        break;
                      }
                      case 3: {
                        m.ch = Mouse::Release;
                        break;
                      }
                      default: {
                        _ctx = Null{{&_buf[_pos_read], 5}};
                        break;
                      }
                    }
                    _pos_read += 5;
                    break;
                  }
                  default: {
                    _ctx = Null{{&_buf[_pos_read], 4}};
                    _pos_read += 4;
                    break;
                  }
                }
              }
            }
            else {
              // escape
              _ctx = Key{{&_buf[_pos_read], 1}, static_cast<char32_t>(_buf[_pos_read])};
              _pos_read += 1;
            }
          }
        }
      }

      if (_on_read) {
        _on_read(_ctx);
      }
      _pos_write = 0;
      _buf_size = 0;
      return;

      read_more:
      _pos_write = _buf_size - _pos_read;
      if (_pos_write >= _buf_max) {
        throw std::runtime_error("read buffer full");
      }
      std::size_t idx {0};
      while (idx + _pos_read < _buf_size) {
        _buf[idx] = _buf[idx + _pos_read];
        ++idx;
      }
    }
    catch (error_code const& e) {
      throw std::runtime_error("read failed");
    }
    catch (std::exception const& e) {
      throw e;
    }
    catch (...) {
      throw;
    }
  }

  asio::io_context& _io;
  asio::posix::stream_descriptor _istream {_io, dup(STDIN_FILENO)};
  fn_on_read _on_read {nullptr};
  Ctx _ctx {Null()};
  std::size_t static constexpr _buf_max {1024};
  char _buf[_buf_max];
  std::size_t _buf_size {0};
  std::size_t _pos_write {0};
  std::chrono::milliseconds _interval {5};
}; // class Read

} // namespace IO
#endif // BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR


} // namespace OB::Belle

#endif // OB_BELLE_IO_HH
