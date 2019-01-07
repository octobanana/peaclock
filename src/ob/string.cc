#include "ob/string.hh"

#include <cstddef>

#include <string>
#include <string_view>
#include <vector>
#include <limits>
#include <utility>

namespace OB::String
{

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
