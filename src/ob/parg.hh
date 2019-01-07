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

#include "ob/string.hh"

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

namespace OB
{

class Parg
{
public:

  Parg()
  {
  }

  Parg(int _argc, char** _argv)
  {
    argc_ = _argc;
    argvf(_argv);
  }

  Parg& name(std::string const _name)
  {
    name_ = _name;
    return *this;
  }

  std::string name() const
  {
    return name_;
  }

  Parg& version(std::string const _version)
  {
    version_ = _version;
    return *this;
  }

  std::string version() const
  {
    return version_;
  }

  Parg& usage(std::string const _usage)
  {
    usage_ += "  " + name_ + " " + _usage + "\n";
    return *this;
  }

  std::string usage() const
  {
    return usage_;
  }

  Parg& description(std::string const _description)
  {
    description_ = _description;
    return *this;
  }

  std::string description() const
  {
    return description_;
  }

  Parg& info(std::string const _title, std::vector<std::string> const _text)
  {
    info_.emplace_back(info_pair{_title, _text});
    return *this;
  }

  Parg& author(std::string const _author)
  {
    author_ = _author;
    return *this;
  }

  std::string author() const
  {
    return author_;
  }

  std::string help() const
  {
    std::stringstream out;
    if (! description_.empty())
    {
      out << name_ << ":" << "\n";
      std::stringstream ss;
      out << "  " << description_ << "\n";
      out << ss.str() << "\n";
    }

    if (! usage_.empty())
    {
      out << "Usage: " << "\n"
        << usage_ << "\n";
    }

    if (! modes_.empty())
    {
      out << "Flags: " << "\n"
        << modes_;
    }

    if (! options_.empty())
    {
      out << "\nOptions: " << "\n"
        << options_;
    }

    if (! info_.empty())
    {
      for (auto const& e : info_)
      {
        out << "\n" << e.title << ":" << "\n";

        for (auto const& t : e.text)
        {
          out << "  " << t << "\n";
        }
      }
    }

    if (! author_.empty())
    {
      out << "\nAuthor: " << "\n";
      std::stringstream ss;
      ss << "  " << author_ << "\n";
      out << ss.str();
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

  int parse(std::string str)
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
        std::size_t start {i};
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
        std::size_t start {i};
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

  void set(std::string _name, std::string _info)
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
      modes_.append("  -" + _short + ", --" + _long + "\n");
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
        modes_.append("  -" + _name + "\n");
      }
      else
      {
        // long
        data_[_name].long_ = _name;
        data_[_name].mode_ = true;
        data_[_name].value_ = "0";
        modes_.append("  --" + _name + "\n");
      }
    }

    std::stringstream out;
    out << "    " << _info << "\n";
    modes_.append(out.str());
  }

  void set(std::string _name, std::string _default, std::string _arg, std::string _info)
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
      options_.append("  -" + _short + ", --" + _long + "=<" + _arg + ">\n");
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
        options_.append("  -" + _name + "=<" + _arg + ">\n");
      }
      else
      {
        // long
        data_[_name].long_ = _name;
        data_[_name].mode_ = false;
        data_[_name].value_ = _default;
        options_.append("  --" + _name + "=<" + _arg + ">\n");
      }
    }

    std::stringstream out;
    out << "    " << _info << "\n";
    options_.append(out.str());
  }

  template<class T>
  T get(std::string const _key)
  {
    static_assert(! std::is_same_v<T, std::string>, "use non-template version of function for 'std::string' type");
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

  std::string get(std::string const _key)
  {
    if (data_.find(_key) == data_.end())
    {
      throw std::logic_error("parg get '" + _key + "' is not defined");
    }
    return data_[_key].value_;
  }

  bool find(std::string const _key) const
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
    return error_;
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
    std::string short_;
    std::string long_;
    bool mode_;
    std::string value_;
    bool seen_ {false};
  };

  struct info_pair
  {
    std::string title;
    std::vector<std::string> text;
  };

private:

  int argc_ {0};
  std::vector<std::string> argv_;
  std::string name_;
  std::string version_;
  std::string usage_;
  std::string description_;
  std::string modes_;
  std::string options_;
  int options_indent_ {0};
  std::vector<info_pair> info_;
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

  std::vector<std::string> delimit(const std::string str, const std::string delim) const
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

  int parse_args(int _argc, std::vector<std::string> _argv)
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

    size_t const similar__max {3};
    if (similar_.size() > similar__max)
    {
      similar_.erase(similar_.begin() + similar__max, similar_.end());
    }
  }

}; // class Parg

} // namespace OB

#endif // OB_PARG_HH
