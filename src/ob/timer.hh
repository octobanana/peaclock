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

Licensed under the MIT License

Copyright (c) 2019 Brett Robinson <https://octobanana.com/>

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

#ifndef OB_TIMER_HH
#define OB_TIMER_HH

#include "ob/string.hh"

#include <ctime>

#include <tuple>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace OB
{

template<typename Clock = std::chrono::steady_clock>
class Timer
{
public:

  Timer() = default;

  operator bool()
  {
    return _is_running;
  }

  Timer& start()
  {
    start(Clock::now());

    return *this;
  }

  Timer& start(std::chrono::time_point<Clock> const& now)
  {
    _is_running = true;
    _start = now;
    _begin = _start;

    return *this;
  }

  Timer& stop()
  {
    update();
    _is_running = false;

    return *this;
  }

  Timer& clear()
  {
    _is_running = false;
    _total = {};
    _start = {};
    _begin = {};
    _end = {};

    return *this;
  }

  Timer& toggle()
  {
    if (_is_running)
    {
      stop();
    }
    else
    {
      start();
    }

    return *this;
  }

  std::chrono::time_point<Clock> begin()
  {
    return _begin;
  }

  std::chrono::time_point<Clock> end()
  {
    return _end;
  }

  template<typename T>
  T time()
  {
    if (_is_running)
    {
      update();
    }

    return std::chrono::time_point_cast<T>(_total).time_since_epoch();
  }

  std::tuple<int, int, int> diff(long int const sec)
  {
    if (_is_running)
    {
      update();
    }

    long int const now {std::chrono::time_point_cast<std::chrono::seconds>(_total).time_since_epoch().count()};

    return seconds_to_hms(sec - now);
  }

  long int seconds()
  {
    if (_is_running)
    {
      update();
    }

    return std::chrono::time_point_cast<std::chrono::seconds>(_total).time_since_epoch().count();
  }

  template<typename T>
  std::size_t get()
  {
    if (_is_running)
    {
      update();
    }

    return static_cast<std::size_t>(std::chrono::time_point_cast<T>(_total).time_since_epoch().count());
  }

  std::tuple<int, int, int> hms()
  {
    if (_is_running)
    {
      update();
    }

    long int const diff {std::chrono::time_point_cast<std::chrono::seconds>(_total).time_since_epoch().count()};

    return seconds_to_hms(diff);
  }

  std::string str()
  {
    if (_is_running)
    {
      update();
    }

    long int const diff {std::chrono::time_point_cast<std::chrono::seconds>(_total).time_since_epoch().count()};

    return seconds_to_string(diff);
  }

  void str(std::string const& str)
  {
    clear();
    _total = std::chrono::time_point<Clock>(string_to_seconds(str));
  }

  static long int str_to_sec(std::string const& str)
  {
    return std::chrono::time_point<std::chrono::seconds>(string_to_seconds(str)).time_since_epoch().count();
  }

  static std::string sec_to_str(long int const sec)
  {
    return seconds_to_string(sec);
  }

private:

  inline static long int constexpr t_second {1};
  inline static long int constexpr t_minute {t_second * 60};
  inline static long int constexpr t_hour   {t_minute * 60};
  inline static long int constexpr t_day    {t_hour * 24};
  inline static long int constexpr t_week   {t_day * 7};
  inline static long int constexpr t_month  {static_cast<long int>(t_day * 30.4)};
  inline static long int constexpr t_year   {t_month * 12};

  static std::chrono::seconds string_to_seconds(std::string const& str)
  {
    auto const pstr = OB::String::match(str,
      std::regex("^(?:(\\d+)Y)?:?(?:(\\d+)M)?:?(?:(\\d+)W)?:?(?:(\\d+)D)?:?(?:(\\d+)h)?:?(?:(\\d+)m)?:?(?:(\\d+)s)?$"));

    if (! pstr)
    {
      return static_cast<std::chrono::seconds>(0);
    }

    auto const t = pstr.value();
    long int sec {0};

    if (! t.at(1).empty())
    {
      sec += std::stol(t.at(1)) * t_year;
    }

    if (! t.at(2).empty())
    {
      sec += std::stol(t.at(2)) * t_month;
    }

    if (! t.at(3).empty())
    {
      sec += std::stol(t.at(3)) * t_week;
    }

    if (! t.at(4).empty())
    {
      sec += std::stol(t.at(4)) * t_day;
    }

    if (! t.at(5).empty())
    {
      sec += std::stol(t.at(5)) * t_hour;
    }

    if (! t.at(6).empty())
    {
      sec += std::stol(t.at(6)) * t_minute;
    }

    if (! t.at(7).empty())
    {
      sec += std::stol(t.at(7)) * t_second;
    }

    return static_cast<std::chrono::seconds>(sec);
  }

  static std::string seconds_to_string(long int sec)
  {
    std::string res;

    auto const fuzzy_string = [&](long int const time_ref, std::string const time_str)
    {
      auto const t = (sec / time_ref);
      sec -= (t * time_ref);
      res += std::to_string(t) + time_str + ":";
    };

    if (sec >= t_year)
    {
      fuzzy_string(t_year, "Y");
    }
    if (sec >= t_month)
    {
      fuzzy_string(t_month, "M");
    }
    if (sec >= t_week)
    {
      fuzzy_string(t_week, "W");
    }
    if (sec >= t_day)
    {
      fuzzy_string(t_day, "D");
    }
    if (sec >= t_hour)
    {
      fuzzy_string(t_hour, "h");
    }
    if (sec >= t_minute)
    {
      fuzzy_string(t_minute, "m");
    }
    if (sec >= t_second)
    {
      fuzzy_string(t_second, "s");
    }
    else
    {
      res += "0s:";
    }

    res.pop_back();

    return res;
  }

  static std::tuple<int, int, int> seconds_to_hms(long int sec)
  {
    int hour {0};
    int minute {0};
    int second {0};

    auto const calc = [&](long int const time_ref)
    {
      auto const t = (sec / time_ref);
      sec -= (t * time_ref);
      return t;
    };

    if (sec > 0)
    {
      if (sec >= t_year)
      {
        calc(t_year);
      }
      if (sec >= t_month)
      {
        calc(t_month);
      }
      if (sec >= t_week)
      {
        calc(t_week);
      }
      if (sec >= t_day)
      {
        calc(t_day);
      }
      if (sec >= t_hour)
      {
        hour = calc(t_hour);
      }
      if (sec >= t_minute)
      {
        minute = calc(t_minute);
      }
      if (sec >= t_second)
      {
        second = calc(t_second);
      }
    }

    return {hour, minute, second};
  }

  void update()
  {
    auto const stop = Clock::now();
    _end = stop;

    if (stop >= _start)
    {
      _total += (stop - _start);
    }

    _start = stop;
  }

  bool _is_running {false};
  std::chrono::time_point<Clock> _start;
  std::chrono::time_point<Clock> _total;
  std::chrono::time_point<Clock> _begin;
  std::chrono::time_point<Clock> _end;
}; // class Timer

} // namespace OB

#endif // OB_TIMER_HH
