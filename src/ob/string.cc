#include "ob/string.hh"

#include <cstddef>

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>
#include <limits>
#include <utility>
#include <optional>
#include <regex>

namespace OB::String
{

std::string to_string(double const val, int const pre)
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(pre) << val;

  return ss.str();
}

std::vector<std::string> split(std::string const& str,
  std::string const& delim, std::size_t size)
{
  std::vector<std::string> vtok;
  std::size_t start {0};
  auto end = str.find(delim);

  while ((size-- > 0) && (end != std::string::npos))
  {
    vtok.emplace_back(str.substr(start, end - start));
    start = end + delim.size();
    end = str.find(delim, start);
  }

  vtok.emplace_back(str.substr(start, end));

  return vtok;
}

std::vector<std::string_view> split_view(std::string_view str,
  std::string_view delim, std::size_t size)
{
  std::vector<std::string_view> vtok;
  std::size_t start {0};
  auto end = str.find(delim);

  while ((size-- > 0) && (end != std::string_view::npos))
  {
    vtok.emplace_back(str.data() + start, end - start);
    start = end + delim.size();
    end = str.find(delim, start);
  }

  vtok.emplace_back(str.data() + start, str.size() - start);

  return vtok;
}

std::string lowercase(std::string const& str)
{
  auto const to_lower = [](char& c)
  {
    if (c >= 'A' && c <= 'Z')
    {
      c += 'a' - 'A';
    }

    return c;
  };

  std::string res {str};

  for (char& c : res)
  {
    c = to_lower(c);
  }

  return res;
}

std::string trim(std::string str)
{
  auto start = str.find_first_not_of(" \t\n\r\f\v");

  if (start != std::string::npos)
  {
    auto end = str.find_last_not_of(" \t\n\r\f\v");
    str = str.substr(start, end - start + 1);

    return str;
  }

  return {};
}

bool assert_rx(std::string const& str, std::regex rx)
{
  std::smatch m;

  if (std::regex_match(str, m, rx, std::regex_constants::match_not_null))
  {
    return true;
  }

  return false;
}

std::optional<std::vector<std::string>> match(std::string const& str, std::regex rx)
{
  std::smatch m;

  if (std::regex_match(str, m, rx, std::regex_constants::match_not_null))
  {
    std::vector<std::string> v;

    for (auto const& e : m)
    {
      v.emplace_back(std::string(e));
    }

    return v;
  }

  return {};
}

std::string repeat(std::size_t const num, std::string const& str)
{
  if (num == 0)
  {
    return {};
  }

  if (num == 1)
  {
    return str;
  }

  std::string res;
  res.reserve(str.size() * num);

  for (std::size_t i {0}; i < num; ++i)
  {
    res += str;
  }

  return res;
}

bool starts_with(std::string const& str, std::string const& val)
{
  if (str.empty() || str.size() < val.size())
  {
    return false;
  }

  if (str.compare(0, val.size(), val) == 0)
  {
    return true;
  }

  return false;
}

std::string escape(std::string str)
{
  for (std::size_t pos = 0; (pos = str.find_first_of("\n\t\r\a\b\f\v\"\'\?", pos)); ++pos)
  {
    if (pos == std::string::npos)
    {
      break;
    }

    switch (str.at(pos))
    {
      case '\n':
      {
        str.replace(pos++, 1, "\\n");
        break;
      }
      case '\t':
      {
        str.replace(pos++, 1, "\\t");
        break;
      }
      case '\r':
      {
        str.replace(pos++, 1, "\\r");
        break;
      }
      case '\a':
      {
        str.replace(pos++, 1, "\\a");
        break;
      }
      case '\b':
      {
        str.replace(pos++, 1, "\\b");
        break;
      }
      case '\f':
      {
        str.replace(pos++, 1, "\\f");
        break;
      }
      case '\v':
      {
        str.replace(pos++, 1, "\\v");
        break;
      }
      case '\?':
      {
        str.replace(pos++, 1, "\\?");
        break;
      }
      case '\'':
      {
        str.replace(pos++, 1, "\\'");
        break;
      }
      case '"':
      {
        str.replace(pos++, 1, "\\\"");
        break;
      }
      default:
      {
        break;
      }
    }
  }

  return str;
}

std::string unescape(std::string str)
{
  for (std::size_t pos = 0; (pos = str.find("\\", pos)); ++pos)
  {
    if (pos == std::string::npos || pos + 1 == std::string::npos)
    {
      break;
    }

    switch (str.at(pos + 1))
    {
      case 'n':
      {
        str.replace(pos, 2, "\n");
        break;
      }
      case 't':
      {
        str.replace(pos, 2, "\t");
        break;
      }
      case 'r':
      {
        str.replace(pos, 2, "\r");
        break;
      }
      case 'a':
      {
        str.replace(pos, 2, "\a");
        break;
      }
      case 'b':
      {
        str.replace(pos, 2, "\b");
        break;
      }
      case 'f':
      {
        str.replace(pos, 2, "\f");
        break;
      }
      case 'v':
      {
        str.replace(pos, 2, "\v");
        break;
      }
      case '?':
      {
        str.replace(pos, 2, "\?");
        break;
      }
      case '\'':
      {
        str.replace(pos, 2, "'");
        break;
      }
      case '"':
      {
        str.replace(pos, 2, "\"");
        break;
      }
      default:
      {
        break;
      }
    }
  }

  return str;
}

std::size_t count(std::string const& str, std::string const& val)
{
  std::size_t pos {0};
  std::size_t count {0};

  for (;;)
  {
    pos = str.find(val, pos);

    if (pos == std::string::npos)
    {
      break;
    }

    ++count;
    ++pos;
  }

  return count;
}

std::size_t damerau_levenshtein(std::string const& lhs, std::string const& rhs,
  std::size_t const weight_insert, std::size_t const weight_substitute,
  std::size_t const weight_delete, std::size_t const weight_transpose)
{
  if (lhs == rhs)
  {
    return 0;
  }

  std::string_view lhsv {lhs};
  std::string_view rhsv {rhs};

  bool swapped {false};
  if (lhsv.size() > rhsv.size())
  {
    swapped = true;
    std::swap(lhsv, rhsv);
  }

  for (std::size_t i = 0; i < lhsv.size(); ++i)
  {
    if (lhsv.at(i) != rhsv.at(i))
    {
      if (i)
      {
        lhsv.substr(i);
        rhsv.substr(i);
      }

      break;
    }
  }

  for (std::size_t i = 0; i < lhsv.size(); ++i)
  {
    if (lhsv.at(lhsv.size() - 1 - i) != rhsv.at(rhsv.size() - 1 - i))
    {
      if (i)
      {
        lhsv.substr(0, lhsv.size() - 1 - i);
        rhsv.substr(0, rhsv.size() - 1 - i);
      }

      break;
    }
  }

  if (swapped)
  {
    std::swap(lhsv, rhsv);
  }

  if (lhsv.empty())
  {
    return rhsv.size() * weight_insert;
  }

  if (rhsv.empty())
  {
    return lhsv.size() * weight_delete;
  }

  std::vector<std::size_t> v0 (rhsv.size() + 1, 0);
  std::vector<std::size_t> v1 (rhsv.size() + 1, 0);
  std::vector<std::size_t> v2 (rhsv.size() + 1, 0);

  for (std::size_t i = 0; i <= rhsv.size(); ++i)
  {
    v1.at(i) = i * weight_insert;
  }

  for (std::size_t i = 0; i < lhsv.size(); ++i)
  {
    v2.at(0) = (i + 1) * weight_delete;
    for (std::size_t j = 0; j < rhsv.size(); j++)
    {
      v2.at(j + 1) = std::min(
        // deletion
        v1.at(j + 1) + weight_delete,
        std::min(
          // insertion
          v2.at(j) + weight_insert,
          // substitution
          v1.at(j) + (weight_substitute * (lhsv.at(i) != rhsv.at(j)))));

      if (i > 0 && j > 0 &&
        (lhsv.at(i - 1) == rhsv.at(j)) &&
        (lhsv.at(i) == rhsv.at(j - 1)))
      {
        v2.at(j + 1) = std::min(
          v0.at(j + 1),
          // transposition
          v0.at(j - 1) + weight_transpose);
      }
    }

    std::swap(v0, v1);
    std::swap(v1, v2);
  }

  return v1.at(rhsv.size());
}

} // namespace OB::String
