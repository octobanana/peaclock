//
// MIT License
//
// Copyright (c) 2018-2019 Brett Robinson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef OB_PARG_HH
#define OB_PARG_HH

#include "ob/algorithm.hh"
#include "ob/string.hh"
#include "ob/term.hh"
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

#include <unistd.h>

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cassert>

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <regex>
#include <algorithm>
#include <type_traits>
#include <filesystem>
#include <functional>

namespace OB
{

class Parg
{
public:

  struct Style
  {
    std::string h1 {aec::fg_magenta + aec::underline};
    std::string h2 {aec::fg_green};
    std::string p {aec::fg_white_bright};
    std::string pa {aec::fg_white};
    std::string opt {aec::fg_green};
    std::string arg {aec::fg_yellow};
    std::string val {aec::fg_cyan};
    std::string success {aec::fg_green};
    std::string error {aec::fg_red};
  } style;

  struct Section
  {
    std::string head;
    std::vector<std::pair<std::string, std::string>> body;
  };

  struct Section_Command
  {
    std::string head;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> body;
  };

  using Output = std::function<void(OB::Term::ostream&, Style const&)>;

  Parg()
  {
  }

  Parg(int _argc, char** _argv)
  {
    argc_ = _argc;
    argvf(_argv);
  }

  bool color()
  {
    return color_;
  }

  Parg& color(bool const val)
  {
    color_ = val;
    return *this;
  }

  Parg& name(std::string const& _name)
  {
    name_ = _name;
    return *this;
  }

  std::string name() const
  {
    return name_;
  }

  Parg& version(std::string const& _version)
  {
    version_ = _version;
    return *this;
  }

  std::string version() const
  {
    if (version_.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << aec::wrap(name_, style.h2)
    << " "
    << aec::wrap(version_, style.p)
#ifdef DEBUG
    << " "
    << aec::wrap("DEBUG", style.error)
#endif
    << "\n";

    return out.str();
  }

  Parg& usage(std::string const& _usage)
  {
    usage_.emplace_back(name_ + " " + _usage);
    return *this;
  }

  std::string usage() const
  {
    if (usage_.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os << aec::wrap("Usage", style.h1) << iom::push();

    bool alt {false};

    for (auto const& e : usage_)
    {
      os << aec::wrap(e, (alt ? style.pa : style.p)) << "\n";

      alt = ! alt;
    }

    os << iom::pop();

    return out.str();
  }

  Parg& description(std::string const& _description)
  {
    description_ = _description;
    return *this;
  }

  std::string description() const
  {
    return description_;
  }

  Parg& info(Output const& out)
  {
    info_.emplace_back(out);
    return *this;
  }

  Parg& info(Section const& s)
  {
    info([s](auto& os, auto const& st)
    {
      os << aec::wrap(s.head, st.h1) << iom::push();

      bool alt {false};

      for (auto const& [k, v] : s.body)
      {
        if (k.empty())
        {
          os << aec::wrap(v, (alt ? st.pa : st.p)) << "\n";
        }
        else
        {
          OB::Algorithm::for_each(OB::String::split_view(k, ", "),
          [&os, &st](auto const& obj)
          {
            os << st.h2 << obj << st.p << ", ";
          },
          [&os, &st](auto const& obj)
          {
            os << st.h2 << obj;
          });

          os
          << aec::clear << iom::push()
          << aec::wrap(v, (alt ? st.pa : st.p)) << iom::pop();
        }

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    });

    return *this;
  }

  Parg& info(Section_Command const& s)
  {
    info([s](auto& os, auto const& st)
    {
      os << aec::wrap(s.head, st.h1) << iom::push();

      bool alt {false};

      for (auto const& [k, v] : s.body)
      {
        auto const cmd = OB::String::split_view(k, " ", 2);

        os
        << iom::word_break(false)
        << aec::wrap(cmd.at(0), st.h2);

        if (cmd.size() == 2)
        {
          os << " " << aec::wrap(cmd.at(1), (v.at(0).first.empty() ? st.arg : st.val));
        }
        else if (cmd.size() >= 3)
        {
          os
          << " " << aec::wrap(cmd.at(1), st.val)
          << " " << aec::wrap(cmd.at(2), st.arg);
        }

        os
        << iom::word_break(true)
        << iom::push();

        for (auto const& [k1, v1] : v)
        {
          if (k1.empty())
          {
            os << aec::wrap(v1, (alt ? st.pa : st.p)) << iom::pop();
          }
          else
          {
            os
            << aec::wrap(k1, st.val) << iom::push()
            << aec::wrap(v1, (alt ? st.pa : st.p)) << iom::pop();
          }

          alt = ! alt;
        }

        if (! v.at(0).first.empty())
        {
          os << iom::pop();
        }
      }

      os << "\n" << iom::pop();
    });

    return *this;
  }

  Parg& author(std::string const& _author)
  {
    author_ = _author;
    return *this;
  }

  std::string author() const
  {
    return author_;
  }

  std::string license() const
  {
    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << aec::wrap("MIT License", style.h1)
    << "\n\n"
    << aec::wrap("Copyright (c) 2019 Brett Robinson", style.h2)
    << "\n\n"
    << iom::first_wrap(true)
    << style.p
    << R"(Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.)"
    << "\n";

    return out.str();
  }

  void ostream_init(OB::Term::ostream& os) const
  {
    if (OB::Term::is_term(STDOUT_FILENO))
    {
      std::size_t width {0};
      OB::Term::width(width, STDOUT_FILENO);
      os.width(width);
      os.line_wrap(true);
    }
    else
    {
      os.line_wrap(false);
    }

    os.escape_codes(color_);
    os.indent(2);
    os.first_wrap(false);
    os.white_space(false);
  }

  std::string help() const
  {
    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    if (! description_.empty())
    {
      os
      << aec::wrap(name_, style.h1) << iom::push()
      << aec::wrap(description_, style.p) << iom::pop();
    }

    if (! usage_.empty())
    {
      os
      << "\n"
      << aec::wrap("Usage", style.h1) << iom::push();

      bool alt {false};

      for (auto const& e : usage_)
      {
        os << aec::wrap(e, (alt ? style.pa : style.p)) << "\n";

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    }

    // options
    if (! data_.empty())
    {
      os << aec::wrap("Options", style.h1) << iom::push();

      bool alt {false};

      for (auto const& v : data_)
      {
        auto const& e = v.second;

        if (! e.short_.empty())
        {
          os << aec::wrap("-" + e.short_, style.opt);
        }

        if (! e.short_.empty() && ! e.long_.empty())
        {
          os << aec::wrap(", ", style.p);
        }

        if (! e.long_.empty())
        {
          os << aec::wrap("--" + e.long_, style.opt);
        }

        if (! e.mode_)
        {
          os
          << aec::wrap("=", style.p)
          << aec::wrap("<" + e.arg_ + ">", style.arg);
        }

        os
        << iom::push()
        << aec::wrap(e.info_, (alt ? style.pa : style.p))
        << iom::pop();

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    }

    if (! info_.empty())
    {
      for (auto const& e : info_)
      {
        e(os, style);
      }
    }

    if (! author_.empty())
    {
      os
      << aec::wrap("Author", style.h1) << iom::push()
      << aec::wrap(author_, style.p) << iom::pop();
    }

    return out.str();
  }

  int parse()
  {
    if (is_stdin_)
    {
      pipe_stdin();
    }
    status_ = parse_args(argc_, argv_);
    return status_;
  }

  int parse(int argc, char** argv)
  {
    if (is_stdin_)
    {
      pipe_stdin();
    }
    argc_ = argc;
    argvf(argv);
    status_ = parse_args(argc_, argv_);
    return status_;
  }

  int parse(std::string const& str)
  {
    auto args = str_to_args(str);
    status_ = parse_args(args.size(), args);
    return status_;
  }

  std::vector<std::string> str_to_args(std::string const& str)
  {
    std::vector<std::string> args;

    std::string const backslash {"\\"};

    // parse str into arg vector as if it was parsed by the shell
    for (std::size_t i = 0; i < str.size(); ++i)
    {
      std::string e {str.at(i)};

      // default
      if (e.find_first_not_of(" \n\t\"'") != std::string::npos)
      {
        bool escaped {false};
        args.emplace_back("");
        for (;i < str.size(); ++i)
        {
          e = str.at(i);
          if (! escaped && e.find_first_of(" \n\t") != std::string::npos)
          {
            --i; // put back unmatched char
            break;
          }
          else if (e == backslash)
          {
            escaped = true;
          }
          else if (escaped)
          {
            args.back() += e;
            escaped = false;
          }
          else
          {
            args.back() += e;
          }
        }
        continue;
      }

      // whitespace
      else if (e.find_first_of(" \n\t") != std::string::npos)
      {
        for (;i < str.size(); ++i)
        {
          e = str.at(i);
          if (e.find_first_not_of(" \n\t") != std::string::npos)
          {
            --i; // put back unmatched char
            break;
          }
        }
        continue;
      }

      // string
      else if (e.find_first_of("\"'") != std::string::npos)
      {
        std::string quote {e};
        bool escaped {false};
        ++i; // skip start quote
        args.emplace_back("");
        for (;i < str.size(); ++i)
        {
          e = str.at(i);
          if (! escaped && e == quote)
          {
            break;
            // skip end quote
          }
          else if (e == backslash)
          {
            escaped = true;
          }
          else if (escaped)
          {
            args.back() += e;
            escaped = false;
          }
          else
          {
            args.back() += e;
          }
        }
      }
    }
    return args;
  }

  void set(std::string const& _name, std::string const& _info)
  {
    // sets a flag
    std::string delim {","};
    if (_name.find(delim) != std::string::npos)
    {
      // short and long
      bool has_short {false};
      std::string _long;
      std::string _short;

      auto const names = delimit(_name, delim);
      assert(names.size() >= 1 && names.size() <= 2);
      _long = names.at(0);

      if (names.size() == 2) has_short = true;
      if (has_short)
      {
        // short name must be one char
        assert(names.at(1).size() == 1);
        _short = names.at(1);
      }

      flags_[_short] = _long;
      data_[_long].long_ = _long;
      data_[_long].short_ = _short;
      data_[_long].mode_ = true;
      data_[_long].value_ = "0";
      data_[_long].info_ = _info;
    }
    else
    {
      if (_name.size() == 1)
      {
        // short
        flags_[_name] = _name;
        data_[_name].long_ = _name;
        data_[_name].short_ = _name;
        data_[_name].mode_ = true;
        data_[_name].value_ = "0";
        data_[_name].info_ = _info;
      }
      else
      {
        // long
        data_[_name].long_ = _name;
        data_[_name].mode_ = true;
        data_[_name].value_ = "0";
        data_[_name].info_ = _info;
      }
    }
  }

  void set(std::string const& _name, std::string const& _default,
    std::string const& _arg, std::string const& _info)
  {
    // sets an option
    std::string delim {","};
    if (_name.find(delim) != std::string::npos)
    {
      bool has_short {false};
      std::string _long;
      std::string _short;

      auto const names = delimit(_name, delim);
      assert(names.size() >= 1 && names.size() <= 2);
      _long = names.at(0);

      if (names.size() == 2) has_short = true;
      if (has_short)
      {
        // short name must be one char
        assert(names.at(1).size() == 1);
        _short = names.at(1);
      }

      flags_[_short] = _long;
      data_[_long].long_ = _long;
      data_[_long].short_ = _short;
      data_[_long].mode_ = false;
      data_[_long].value_ = _default;
      data_[_long].arg_ = _arg;
      data_[_long].info_ = _info;
    }
    else
    {
      if (_name.size() == 1)
      {
        // short
        flags_[_name] = _name;
        data_[_name].long_ = _name;
        data_[_name].short_ = _name;
        data_[_name].mode_ = false;
        data_[_name].value_ = _default;
        data_[_name].arg_ = _arg;
        data_[_name].info_ = _info;
      }
      else
      {
        // long
        data_[_name].long_ = _name;
        data_[_name].mode_ = false;
        data_[_name].value_ = _default;
        data_[_name].arg_ = _arg;
        data_[_name].info_ = _info;
      }
    }
  }

  template<typename T,
    std::enable_if_t<
      std::is_same_v<T, std::string> ||
      std::is_same_v<T, std::filesystem::path>,
      int> = 0>
  T get(std::string const& _key)
  {
    if (data_.find(_key) == data_.end())
    {
      throw std::logic_error("parg get '" + _key + "' is not defined");
    }
    return data_[_key].value_;
  }

  template<typename T,
    std::enable_if_t<
      std::is_integral_v<T>,
      int> = 0>
  T get(std::string const& _key)
  {
    if (data_.find(_key) == data_.end())
    {
      throw std::logic_error("parg get '" + _key + "' is not defined");
    }
    std::stringstream ss;
    ss << data_[_key].value_;
    T val;
    ss >> val;
    return val;
  }

  bool find(std::string const& _key) const
  {
    // key must exist
    if (data_.find(_key) == data_.end()) return false;
    return data_.at(_key).seen_;
  }

  Parg& set_pos(bool const _positional = true)
  {
    is_positional_ = _positional;
    return *this;
  }

  std::string get_pos() const
  {
    std::string str;
    if (positional_vec_.empty())
    {
      return str;
    }
    for (auto const& e : positional_vec_)
    {
      str += e + " ";
    }
    str.pop_back();
    return str;
  }

  std::vector<std::string> get_pos_vec() const
  {
    return positional_vec_;
  }

  Parg& set_stdin(bool const _stdin = true)
  {
    is_stdin_ = _stdin;
    return *this;
  }

  std::string get_stdin() const
  {
    return stdin_;
  }

  int status() const
  {
    return status_;
  }

  std::string error() const
  {
    if (error_.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << iom::first_wrap(false)
    << aec::wrap("Error: ", style.error)
    << error_ << "\n"
    << iom::first_wrap(true);

    auto const similar_names = similar();

    if (similar_names.size() > 0)
    {
      os
      << "\nDid you mean:"
      << style.opt << iom::push();

      OB::Algorithm::for_each(similar_names,
      [&os](auto const& e)
      {
        os << "--" << e << "\n";
      },
      [&os](auto const& e)
      {
        os << "--" << e;
      });

      os << aec::clear << iom::pop();
    }

    return out.str();
  }

  std::vector<std::string> const& similar() const
  {
    return similar_;
  }

  std::size_t flags_found() const
  {
    std::size_t count {0};

    for (auto const& e : data_)
    {
      if (e.second.mode_ && e.second.seen_)
      {
        ++count;
      }
    }

    return count;
  }

  std::size_t options_found() const
  {
    std::size_t count {0};

    for (auto const& e : data_)
    {
      if (! e.second.mode_ && e.second.seen_)
      {
        ++count;
      }
    }

    return count;
  }

  struct Option
  {
    std::string arg_;
    std::string info_;
    std::string short_;
    std::string long_;
    bool mode_;
    std::string value_;
    bool seen_ {false};
  };

private:

  int argc_ {0};
  std::vector<std::string> argv_;
  bool color_ {true};
  std::string name_;
  std::string version_;
  std::vector<std::string> usage_;
  std::string description_;
  int options_indent_ {0};
  std::vector<Output> info_;
  std::string author_;
  std::map<std::string, Option> data_;
  std::map<std::string, std::string> flags_;
  bool is_positional_ {false};
  std::string positional_;
  std::vector<std::string> positional_vec_;
  std::string stdin_;
  bool is_stdin_ {false};
  int status_ {0};
  std::string error_;
  std::vector<std::string> similar_;

  void argvf(char** _argv)
  {
    // removes first arg
    if (argc_ < 1) return;
    for (int i = 1; i < argc_; ++i)
    {
      argv_.emplace_back(_argv[i]);
    }
    // std::cerr << "argv: " << i << " -> " << argv_.at(i) << std::endl;
    --argc_;
  }

  int pipe_stdin()
  {
    if (! isatty(STDIN_FILENO))
    {
      stdin_.assign((std::istreambuf_iterator<char>(std::cin)),
        (std::istreambuf_iterator<char>()));
      return 0;
    }
    stdin_ = "";
    return -1;
  }

  std::vector<std::string> delimit(std::string const& str, std::string const& delim) const
  {
    std::vector<std::string> vtok;
    std::size_t start {0};
    std::size_t end = str.find(delim);
    while (end != std::string::npos) {
      vtok.emplace_back(str.substr(start, end - start));
      start = end + delim.length();
      end = str.find(delim, start);
    }
    vtok.emplace_back(str.substr(start, end));
    return vtok;
  }

  int parse_args(int _argc, std::vector<std::string> const& _argv)
  {
    if (_argc < 1) return 1;

    bool dashdash {false};

    // loop through arg vector
    for (int i = 0; i < _argc; ++i)
    {
      std::string const& tmp {_argv.at(static_cast<std::size_t>(i))};
      // std::cerr << "ARG: " << i << " -> " << tmp << std::endl;

      if (dashdash)
      {
        positional_vec_.emplace_back(tmp);
        continue;
      }

      if (tmp.size() > 1 && tmp.at(0) == '-' && tmp.at(1) != '-')
      {
        // short
        // std::cerr << "SHORT: " << tmp << std::endl;

        std::string c {tmp.at(1)};
        if (flags_.find(c) != flags_.end() && !(data_.at(flags_.at(c)).mode_))
        {
          // short arg
          // std::cerr << "SHORT: arg -> " << c << std::endl;

          if (data_.at(flags_.at(c)).seen_)
          {
            // error
            error_ = "flag '-" + c + "' has already been seen";
            return -1;
          }

          if (tmp.size() > 2 && tmp.at(2) != '=')
          {
            data_.at(flags_.at(c)).value_ = tmp.substr(2, tmp.size() - 1);
            data_.at(flags_.at(c)).seen_ = true;
          }
          else if (tmp.size() > 3 && tmp.at(2) == '=')
          {
            data_.at(flags_.at(c)).value_ = tmp.substr(3, tmp.size() - 1);
            data_.at(flags_.at(c)).seen_ = true;
          }
          else if (i + 1 < _argc)
          {
            data_.at(flags_.at(c)).value_ = _argv.at(static_cast<std::size_t>(i + 1));
            data_.at(flags_.at(c)).seen_ = true;
            ++i;
          }
          else
          {
            // error
            error_ = "flag '-" + c + "' requires an arg";
            return -1;
          }
        }
        else
        {
          // short mode
          for (std::size_t j = 1; j < tmp.size(); ++j)
          {
            std::string s {tmp.at(j)};

            if (flags_.find(s) != flags_.end() && data_.at(flags_.at(s)).mode_)
            {
              // std::cerr << "SHORT: mode -> " << s << std::endl;

              if (data_.at(flags_.at(s)).seen_)
              {
                // error
                error_ = "flag '-" + s + "' has already been seen";
                return -1;
              }

              data_.at(flags_.at(s)).value_ = "1";
              data_.at(flags_.at(s)).seen_ = true;
            }
            else
            {
              // error
              error_ = "invalid flag '" + tmp + "'";
              // find_similar(s);

              return -1;
            }
          }
        }
      }
      else if (tmp.size() > 2 && tmp.at(0) == '-' && tmp.at(1) == '-')
      {
        // long || --
        // std::cerr << "LONG: " << tmp << std::endl;
        std::string c {tmp.substr(2, tmp.size() - 1)};
        std::string a;

        auto const delim = c.find("=");
        if (delim != std::string::npos)
        {
          c = tmp.substr(2, delim);
          a = tmp.substr(3 + delim, tmp.size() - 1);
        }

        if (data_.find(c) != data_.end())
        {
          if (data_.at(c).seen_)
          {
            // error
            error_ = "option '--" + c + "' has already been seen";
            return -1;
          }

          if (data_.at(c).mode_ && a.size() == 0)
          {
            // std::cerr << "LONG: mode -> " << c << std::endl;
            data_.at(c).value_ = "1";
            data_.at(c).seen_ = true;
          }
          else
          {
            // std::cerr << "LONG: arg -> " << c << std::endl;
            if (a.size() > 0)
            {
              data_.at(c).value_ = a;
              data_.at(c).seen_ = true;
            }
            else if (i + 1 < _argc)
            {
              data_.at(c).value_ = _argv.at(static_cast<std::size_t>(i + 1));
              data_.at(c).seen_ = true;
              ++i;
            }
            else
            {
              // error
              error_ = "option '--" + c + "' requires an arg";
              return -1;
            }
          }
        }
        else
        {
          // error
          error_ = "invalid option '" + tmp + "'";
          find_similar(c);
          return -1;
        }
      }
      else if (tmp.size() > 0 && is_positional_)
      {
        // positional
        // std::cerr << "POS: " << tmp << std::endl;
        if (tmp == "--")
        {
          dashdash = true;
        }
        else
        {
          positional_vec_.emplace_back(tmp);
        }
      }
      else
      {
        // error
        error_ = "no match for '" + tmp + "'";
        find_similar(tmp);
        return -1;
      }
    }

    return 0;
  }

  void find_similar(std::string const& name)
  {
    int const weight_max {8};
    std::vector<std::pair<int, std::string>> dist;

    for (auto const& [key, val] : data_)
    {
      int weight {0};

      if (OB::String::starts_with(val.long_, name))
      {
        weight = 0;
      }
      else
      {
        weight = OB::String::damerau_levenshtein(name, val.long_, 1, 2, 3, 0);
      }

      if (weight < weight_max)
      {
        dist.emplace_back(weight, val.long_);
      }
    }

    std::sort(dist.begin(), dist.end(),
    [](auto const& lhs, auto const& rhs)
    {
      return (lhs.first == rhs.first) ?
        (lhs.second.size() < rhs.second.size()) :
        (lhs.first < rhs.first);
    });

    for (auto const& [key, val] : dist)
    {
      similar_.emplace_back(val);
    }

    size_t const similar_max {3};
    if (similar_.size() > similar_max)
    {
      similar_.erase(similar_.begin() + similar_max, similar_.end());
    }
  }

}; // class Parg

} // namespace OB

#endif // OB_PARG_HH
