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

Belle Signal
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

#ifndef OB_BELLE_SIGNAL_HH
#define OB_BELLE_SIGNAL_HH

#include "ob/belle/config.hh"

#include <boost/asio.hpp>
#include <boost/config.hpp>

#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <csignal>

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace OB::Belle {

namespace asio = boost::asio;
using error_code = boost::system::error_code;

class Signal final {
public:

  using fn_on_signal = std::function<void(error_code const&, int)>;

  explicit Signal(asio::io_context& io_) noexcept : _io {io_} {}

  void wait() {
    _set.async_wait(_callback);
  }

  Signal& on_signal(int const sig_, fn_on_signal const fn_) {
    _set.add(sig_);
    _event[sig_] = fn_;
    return *this;
  }

  Signal& on_signal(std::vector<int> const sig_, fn_on_signal const fn_) {
    for (auto const& e : sig_) {
      _set.add(e);
      _event[e] = fn_;
    }
    return *this;
  }

  static std::string str(int sig_) {
    std::string res;
    if (_str.find(sig_) != _str.end()) {
      res = _str.at(sig_);
    }
    return res;
  }

private:

  std::function<void(error_code, int)> const _callback = [&](auto ec_, auto sig_) {
    if (_event.find(sig_) != _event.end()) {
      _event.at(sig_)(ec_, sig_);
    }
  };

  asio::io_context& _io;
  asio::signal_set _set {_io};
  std::unordered_map<int, fn_on_signal> _event;

  static inline std::unordered_map<int, std::string> const _str {
    {SIGHUP, "SIGHUP"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGILL, "SIGILL"},
    {SIGTRAP, "SIGTRAP"},
    {SIGABRT, "SIGABRT"},
    {SIGBUS, "SIGBUS"},
    {SIGFPE, "SIGFPE"},
    {SIGKILL, "SIGKILL"},
    {SIGUSR1, "SIGUSR1"},
    {SIGSEGV, "SIGSEGV"},
    {SIGUSR2, "SIGUSR2"},
    {SIGPIPE, "SIGPIPE"},
    {SIGALRM, "SIGALRM"},
    {SIGTERM, "SIGTERM"},
    {SIGSTKFLT, "SIGSTKFLT"},
    {SIGCHLD, "SIGCHLD"},
    {SIGCONT, "SIGCONT"},
    {SIGSTOP, "SIGSTOP"},
    {SIGTSTP, "SIGTSTP"},
    {SIGTTIN, "SIGTTIN"},
    {SIGTTOU, "SIGTTOU"},
    {SIGURG, "SIGURG"},
    {SIGXCPU, "SIGXCPU"},
    {SIGXFSZ, "SIGXFSZ"},
    {SIGVTALRM, "SIGVTALRM"},
    {SIGPROF, "SIGPROF"},
    {SIGWINCH, "SIGWINCH"},
    {SIGPOLL, "SIGPOLL"},
    {SIGPWR, "SIGPWR"},
    {SIGSYS, "SIGSYS"},
  };
}; // class Signal

} // namespace OB::Belle

#endif // OB_BELLE_SIGNAL_HH
