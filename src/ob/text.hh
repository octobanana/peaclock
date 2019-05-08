#ifndef OB_TEXT_HH
#define OB_TEXT_HH

#define U_CHARSET_IS_UTF8 1

#include <unicode/coll.h>
#include <unicode/regex.h>
#include <unicode/utext.h>
#include <unicode/unistr.h>
#include <unicode/brkiter.h>
#include <unicode/bytestream.h>
#include <unicode/stringpiece.h>
#include <unicode/normalizer2.h>

#include <cstddef>
#include <cstdint>

#include <string>
#include <sstream>
#include <string_view>
#include <array>
#include <vector>
#include <limits>
#include <memory>
#include <utility>
#include <iterator>
#include <algorithm>

namespace OB::Text
{

class View
{
public:

  using size_type = std::size_t;
  using char_type = char;
  using string = std::basic_string<char_type>;
  using string_view = std::basic_string_view<char_type>;
  using brk_iter = icu::BreakIterator;
  using locale = icu::Locale;

  struct Ctx
  {
    Ctx(size_type bytes_, size_type tcols_, size_type cols_, string_view str_) :
      bytes {bytes_},
      tcols {tcols_},
      cols {cols_},
      str {str_}
    {
    }

    friend std::ostream& operator<<(std::ostream& os, Ctx const& obj)
    {
      os << obj.str;

      return os;
    }

    operator string()
    {
      return string(str);
    }

    operator string_view()
    {
      return str;
    }

    size_type bytes {0};
    size_type tcols {0};
    size_type cols {0};
    string_view str {};
  }; // struct Ctx

  using value_type = std::vector<Ctx>;
  using iterator = typename value_type::iterator;
  using const_iterator = typename value_type::const_iterator;
  using reverse_iterator = typename value_type::reverse_iterator;
  using const_reverse_iterator = typename value_type::const_reverse_iterator;

  static auto constexpr iter_end {icu::BreakIterator::DONE};
  static size_type constexpr npos {std::numeric_limits<size_type>::max()};

  View() = default;
  View(View&&) = default;
  View(View const&) = default;

  View(string_view str)
  {
    this->str(str);
  }

  ~View() = default;

  View& operator=(View&&) = default;
  View& operator=(View const&) = default;

  View& operator=(string_view str)
  {
    this->str(str);

    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, View const& obj)
  {
    os << obj.str();

    return os;
  }

  operator string()
  {
    return string(str());
  }

  View& str(string_view str)
  {
    _cols = 0;
    _bytes = 0;

    _view.clear();
    // _view.shrink_to_fit();

    if (str.empty())
    {
      return *this;
    }

    UErrorCode ec = U_ZERO_ERROR;

    std::unique_ptr<UText, decltype(&utext_close)> text (
      utext_openUTF8(nullptr, str.data(), static_cast<std::int64_t>(str.size()), &ec),
      utext_close);

    if (U_FAILURE(ec))
    {
      throw std::runtime_error("failed to create utext");
    }

    std::unique_ptr<brk_iter> iter {brk_iter::createCharacterInstance(
      locale::getDefault(), ec)};

    if (U_FAILURE(ec))
    {
      throw std::runtime_error("failed to create break iterator");
    }

    iter->setText(text.get(), ec);

    if (U_FAILURE(ec))
    {
      throw std::runtime_error("failed to set break iterator text");
    }

    // get size of iterator
    size_type size {0};
    while (iter->next() != iter_end)
    {
      ++size;
    }

    // reserve array size
    _view.reserve(size);

    size = 0;
    UChar32 uch;
    int width {0};
    size_type cols {0};
    auto begin = iter->first();
    auto end = iter->next();

    while (end != iter_end)
    {
      // get column width
      uch = utext_char32At(text.get(), begin);
      width = u_getIntPropertyValue(uch, UCHAR_EAST_ASIAN_WIDTH);
      if (width == U_EA_FULLWIDTH || width == U_EA_WIDE)
      {
        // full width
        cols = 2;
      }
      else
      {
        // half width
        cols = 1;
      }

      // get string size
      size = static_cast<size_type>(end - begin);

      // add character context to array
      _view.emplace_back(_bytes, _cols, cols, string_view(str.data() + (_bytes * sizeof(char_type)), size));

      // increase total column count
      _cols += cols;

      // increase total byte count
      _bytes += size;

      // increase iterators
      begin = end;
      end = iter->next();
    }

    return *this;
  }

  string_view str() const
  {
    if (_view.empty())
    {
      return {};
    }

    return string_view(_view.at(0).str.data(), _bytes);
  }

  value_type const& view() const
  {
    return _view;
  }

  size_type byte_to_char(size_type pos) const
  {
    if (pos >= _bytes)
    {
      return npos;
    }

    auto const it = std::lower_bound(_view.crbegin(), _view.crend(), pos,
      [](auto const& lhs, auto const& rhs) {
        return lhs.bytes > rhs;
      });

    if (it != _view.crend())
    {
      return static_cast<size_type>(std::distance(_view.cbegin(), it.base()) - 1);
    }

    return npos;
  }

  size_type char_to_byte(size_type pos) const
  {
    if (pos >= _view.size())
    {
      return npos;
    }

    auto const& ctx = _view.at(pos);

    return ctx.bytes;
  }

  Ctx& operator[](size_type pos)
  {
    return _view[pos];
  }

  Ctx const& operator[](size_type pos) const
  {
    return _view[pos];
  }

  Ctx& at(size_type pos)
  {
    return _view.at(pos);
  }

  Ctx const& at(size_type pos) const
  {
    return _view.at(pos);
  }

  Ctx& front()
  {
    return _view.front();
  }

  Ctx const& front() const
  {
    return _view.front();
  }

  Ctx& back()
  {
    return _view.back();
  }

  Ctx const& back() const
  {
    return _view.back();
  }

  string_view substr(size_type pos, size_type size = npos) const
  {
    if (pos >= _view.size())
    {
      return {};
    }

    if (size == npos)
    {
      size = _view.size();
    }
    else
    {
      size += pos;
    }

    size_type count {0};

    for (size_type i = pos; i < size && i < _view.size(); ++i)
    {
      count += _view.at(i).str.size();
    }

    return string_view(_view.at(pos).str.data(), count);
  }

  size_type find(string const& str, size_type pos = npos) const
  {
    if (pos == npos)
    {
      pos = 0;
    }

    if (pos >= _view.size())
    {
      return npos;
    }


    for (size_type i = pos; i < _view.size(); ++i)
    {
      if (str == _view.at(i).str)
      {
        return i;
      }
    }

    return npos;
  }

  size_type rfind(string const& str, size_type pos = npos) const
  {
    if (pos == npos)
    {
      pos = _view.size() - 1;
    }

    if (pos == 0 || pos >= _view.size())
    {
      return npos;
    }

    for (size_type i = pos; i != npos; --i)
    {
      if (str == _view.at(i).str)
      {
        return i;
      }
    }

    return npos;
  }

  size_type find_first_of(View const& str, size_type pos = npos) const
  {
    if (pos == npos)
    {
      pos = 0;
    }

    if (pos >= _view.size())
    {
      return npos;
    }

    for (size_type i = pos; i < _view.size(); ++i)
    {
      auto const& lhs = _view.at(i).str;

      for (size_type j = 0; j < str.size(); ++j)
      {
        if (lhs == str.at(j).str)
        {
          return i;
        }
      }
    }

    return npos;
  }

  size_type rfind_first_of(View const& str, size_type pos = npos) const
  {
    if (pos == npos)
    {
      pos = _view.size() - 1;
    }

    if (pos == 0 || pos >= _view.size())
    {
      return npos;
    }

    for (size_type i = pos; i != npos; --i)
    {
      auto const& lhs = _view.at(i).str;

      for (size_type j = 0; j < str.size(); ++j)
      {
        if (lhs == str.at(j).str)
        {
          return i;
        }
      }
    }

    return npos;
  }

  bool empty() const
  {
    return _view.empty();
  }

  View& clear()
  {
    _view.clear();
    _bytes = 0;
    _cols = 0;

    return *this;
  }

  View& shrink_to_fit()
  {
    _view.shrink_to_fit();

    return *this;
  }

  size_type size() const
  {
    return _view.size();
  }

  size_type length() const
  {
    return _view.size();
  }

  size_type bytes() const
  {
    return _bytes;
  }

  size_type bytes(size_type pos, size_type size = npos) const
  {
    if (pos >= _view.size())
    {
      return npos;
    }

    if (size == npos)
    {
      size = _view.size();
    }
    else
    {
      size += pos;
    }

    size_type count {0};

    for (size_type i = pos; i < size && i < _view.size(); ++i)
    {
      count += _view.at(i).str.size();
    }

    return count;
  }

  size_type cols() const
  {
    return _cols;
  }

  size_type cols(size_type pos, size_type size = npos) const
  {
    if (pos >= _view.size())
    {
      return npos;
    }

    if (size == npos)
    {
      size = _view.size();
    }
    else
    {
      size += pos;
    }

    size_type count {0};

    for (size_type i = pos; i < size && i < _view.size(); ++i)
    {
      count += _view.at(i).cols;
    }

    return count;
  }

  string_view colstr(size_type pos, size_type size = npos) const
  {
    if (pos >= _view.size())
    {
      return {};
    }

    if (size == npos)
    {
      size = _view.size();
    }
    else
    {
      size += pos;
    }

    size_type count {0};

    for (size_type i = pos, cols = 0; i < _view.size(); ++i)
    {
      auto const& ctx = _view.at(i);

      if (cols + ctx.cols > size)
      {
        break;
      }

      cols += ctx.cols;
      count += ctx.str.size();
    }

    return string_view(_view.at(pos).str.data(), count);
  }

  string_view rcolstr(size_type pos, size_type size = npos) const
  {
    if (empty())
    {
      return {};
    }

    if (pos >= _view.size())
    {
      pos = _view.size() - 1;
    }

    if (size == npos)
    {
      size = _view.size() - pos;
    }

    size_type count {0};

    for (size_type cols = 0; pos != npos; --pos)
    {
      auto const& ctx = _view.at(pos);

      if (cols + ctx.cols > size)
      {
        ++pos;

        break;
      }

      cols += ctx.cols;
      count += ctx.str.size();
    }

    if (pos == npos)
    {
      ++pos;
    }

    return string_view(_view.at(pos).str.data(), count);
  }

  iterator begin()
  {
    return _view.begin();
  }

  const_iterator cbegin() const
  {
    return _view.cbegin();
  }

  reverse_iterator rbegin()
  {
    return _view.rbegin();
  }

  const_reverse_iterator crbegin() const
  {
    return _view.crbegin();
  }

  iterator end()
  {
    return _view.end();
  }

  const_iterator cend() const
  {
    return _view.cend();
  }

  reverse_iterator rend()
  {
    return _view.rend();
  }

  const_reverse_iterator crend() const
  {
    return _view.crend();
  }

private:

  // array of contexts mapping the string
  value_type _view;

  // number of columns needed to display the string
  size_type _cols {0};

  // number of bytes in the string
  size_type _bytes {0};
}; // class View

class String
{
public:

  using size_type = std::size_t;
  using char_type = char;
  using string = std::basic_string<char_type>;
  using string_view = std::basic_string_view<char_type>;
  using Ctx = View::Ctx;

  using iterator = View::iterator;
  using const_iterator = View::const_iterator;
  using reverse_iterator = View::reverse_iterator;
  using const_reverse_iterator = View::const_reverse_iterator;

  static size_type constexpr npos {std::numeric_limits<size_type>::max()};

  String(string const& str = {}):
    _str {str},
    _view {_str}
  {
  }

  String(String&& obj)
  {
    _str = std::move(obj._str);
    sync();
  }

  String(String const& obj)
  {
    _str = obj._str;
    sync();
  }

  ~String() = default;

  String& operator=(String&& obj)
  {
    _str = std::move(obj._str);
    sync();

    return *this;
  }

  String& operator=(String const& obj)
  {
    _str = obj._str;
    sync();

    return *this;
  }

  String& operator=(string_view str)
  {
    _str = string(str);
    sync();

    return *this;
  }

  String& operator=(string const& str)
  {
    _str = str;
    sync();

    return *this;
  }

  template<typename T>
  String& operator<<(T const& obj)
  {
    std::ostringstream os;
    os << obj;
    append(os.str());

    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, String const& obj)
  {
    os << obj._str;

    return os;
  }

  friend std::istream& operator>>(std::istream& is, String& obj)
  {
    if (is >> obj._str)
    {
      obj.sync();
    }
    else
    {
      is.setstate(std::ios::failbit);
    }

    return is;
  }

  operator string()
  {
    return _str;
  }

  operator string_view()
  {
    return string_view(_str.data(), _str.size());
  }

  operator View()
  {
    return _view;
  }

  string& str()
  {
    return _str;
  }

  string const& str() const
  {
    return _str;
  }

  String& str(string_view str)
  {
    _str = str;
    sync();

    return *this;
  }

  View& view()
  {
    return _view;
  }

  View const& view() const
  {
    return _view;
  }

  String& sync()
  {
    _view.str(_str);

    return *this;
  }

  size_type byte_to_char(size_type pos) const
  {
    return _view.byte_to_char(pos);
  }

  size_type char_to_byte(size_type pos) const
  {
    return _view.char_to_byte(pos);
  }

  String& append(string const& val)
  {
    _str.append(val);
    sync();

    return *this;
  }

  String& insert(size_type pos, string const& val)
  {
    pos = _view.char_to_byte(pos);
    if (pos == npos)
    {
      pos = _str.size();
    }

    _str.insert(pos, val);
    sync();

    return *this;
  }

  String& erase(size_type pos, size_type size)
  {
    auto const get_pos = ([this, &pos]() {
      auto const bpos = _view.char_to_byte(pos);
      if (pos == npos)
      {
        return _str.size();
      }
      return bpos;
    })();

    _str.erase(get_pos, _view.substr(pos, size).size());
    sync();

    return *this;
  }

  String& replace(size_type pos, size_type size, string const& val)
  {
    erase(pos, size);
    insert(pos, val);

    return *this;
  }

  char const* data() const
  {
    return _str.data();
  }

  char* data()
  {
    return _str.data();
  }

  char const* c_str() const
  {
    return _str.data();
  }

  String& reserve(size_type size)
  {
    _str.reserve(size);

    return *this;
  }

  size_type capacity() const
  {
    return _str.capacity();
  }

  size_type max_size() const
  {
    return _str.max_size();
  }

  Ctx& operator[](size_type pos)
  {
    return _view[pos];
  }

  Ctx const& operator[](size_type pos) const
  {
    return _view[pos];
  }

  Ctx& at(size_type pos)
  {
    return _view.at(pos);
  }

  Ctx const& at(size_type pos) const
  {
    return _view.at(pos);
  }

  Ctx& front()
  {
    return _view.front();
  }

  Ctx const& front() const
  {
    return _view.front();
  }

  Ctx& back()
  {
    return _view.back();
  }

  Ctx const& back() const
  {
    return _view.back();
  }

  string_view substr(size_type pos, size_type size = npos) const
  {
    return _view.substr(pos, size);
  }

  size_type find(string const& str, size_type pos = npos) const
  {
    return _view.find(str, pos);
  }

  size_type rfind(string const& str, size_type pos = npos) const
  {
    return _view.rfind(str, pos);
  }

  size_type find_first_of(View const& str, size_type pos = npos) const
  {
    return _view.find_first_of(str, pos);
  }

  size_type rfind_first_of(View const& str, size_type pos = npos) const
  {
    return _view.rfind_first_of(str, pos);
  }

  size_type empty() const
  {
    return _str.empty();
  }

  String& clear()
  {
    _view.clear();
    _str.clear();

    return *this;
  }

  String& shrink_to_fit()
  {
    _view.shrink_to_fit();
    _str.shrink_to_fit();

    return *this;
  }

  size_type size() const
  {
    return _view.size();
  }

  size_type length() const
  {
    return _view.length();
  }

  size_type bytes() const
  {
    return _view.bytes();
  }

  size_type bytes(size_type pos, size_type size = npos) const
  {
    return _view.bytes(pos, size);
  }

  size_type cols() const
  {
    return _view.cols();
  }

  size_type cols(size_type pos, size_type size = npos) const
  {
    return _view.cols(pos, size);
  }

  string_view colstr(size_type pos, size_type size = npos) const
  {
    return _view.colstr(pos, size);
  }

  string_view rcolstr(size_type pos, size_type size = npos) const
  {
    return _view.rcolstr(pos, size);
  }

  iterator begin()
  {
    return _view.begin();
  }

  const_iterator cbegin() const
  {
    return _view.cbegin();
  }

  reverse_iterator rbegin()
  {
    return _view.rbegin();
  }

  const_reverse_iterator crbegin() const
  {
    return _view.crbegin();
  }

  iterator end()
  {
    return _view.end();
  }

  const_iterator cend() const
  {
    return _view.cend();
  }

  reverse_iterator rend()
  {
    return _view.rend();
  }

  const_reverse_iterator crend() const
  {
    return _view.crend();
  }

private:

  string _str;
  View _view;
}; // class String

class Char32
{
public:

  Char32() = default;

  Char32(char32_t val_, std::string const& str_) :
    val {val_},
    str {str_}
  {
  }

  friend std::ostream& operator<<(std::ostream& os, Char32 const& obj)
  {
    os << obj.str;

    return os;
  }

  Char32& clear()
  {
    val = 0;
    str.clear();

    return *this;
  }

  char32_t val {0};
  std::string str;
}; // class Char32

class Regex
{
public:

  using size_type = std::size_t;
  using char_type = char;
  using string = std::basic_string<char_type>;
  using string_view = std::basic_string_view<char_type>;
  using regex = icu::RegexMatcher;

  struct Match
  {
    friend std::ostream& operator<<(std::ostream& os, Match const& obj)
    {
      os << obj.str;

      return os;
    }

    size_type pos {0};
    size_type size {0};
    string_view str;
    std::vector<string_view> group;
  }; // struct Match

  using value_type = std::vector<Match>;
  using iterator = typename value_type::iterator;
  using const_iterator = typename value_type::const_iterator;
  using reverse_iterator = typename value_type::reverse_iterator;
  using const_reverse_iterator = typename value_type::const_reverse_iterator;

  Regex() = default;
  Regex(Regex&&) = default;
  Regex(Regex const&) = default;

  Regex(string_view rx, string_view str)
  {
    match(rx, str);
  }

  ~Regex() = default;

  Regex& operator=(Regex&&) = default;
  Regex& operator=(Regex const&) = default;

  Regex& match(string_view rx, string_view str)
  {
    _str.clear();
    _str.shrink_to_fit();

    if (str.empty())
    {
      return *this;
    }

    UErrorCode ec = U_ZERO_ERROR;

    std::unique_ptr<UText, decltype(&utext_close)> urx (
      utext_openUTF8(nullptr, rx.data(), static_cast<std::int64_t>(rx.size()), &ec),
      utext_close);

    if (U_FAILURE(ec))
    {
      throw std::runtime_error("failed to create utext");
    }

    std::unique_ptr<UText, decltype(&utext_close)> ustr (
      utext_openUTF8(nullptr, str.data(), static_cast<std::int64_t>(str.size()), &ec),
      utext_close);

    if (U_FAILURE(ec))
    {
      throw std::runtime_error("failed to create utext");
    }

    std::unique_ptr<regex> iter {new regex(urx.get(), UREGEX_CASE_INSENSITIVE, ec)};

    if (! U_SUCCESS(ec))
    {
      throw std::runtime_error("failed to create regex matcher");
    }

    iter->reset(ustr.get());

    size_type size {0};
    std::int32_t count {0};
    std::int32_t begin {0};
    std::int32_t end {0};

    while (iter->find())
    {
      count = static_cast<size_type>(iter->groupCount());

      begin = iter->start(ec);

      if (U_FAILURE(ec))
      {
        throw std::runtime_error("failed to get regex matcher start");
      }

      end = iter->end(ec);

      if (U_FAILURE(ec))
      {
        throw std::runtime_error("failed to get regex matcher end");
      }

      size = static_cast<size_type>(end - begin);

      Match match;
      match.pos = static_cast<size_type>(begin);
      match.size = static_cast<size_type>(count);
      match.str = string_view(str.data() + begin, size);

      for (std::int32_t i = 1; i <= count; ++i)
      {
        begin = iter->start(i, ec);

        if (U_FAILURE(ec))
        {
          throw std::runtime_error("failed to get regex matcher group start");
        }

        end = iter->end(i, ec);

        if (U_FAILURE(ec))
        {
          throw std::runtime_error("failed to get regex matcher group end");
        }

        size = static_cast<size_type>(end - begin);

        match.group.emplace_back(string_view(str.data() + begin, size));
      }

      _str.emplace_back(match);
    }

    return *this;
  }

  value_type const& get() const
  {
    return _str;
  }

  Match& at(size_type pos)
  {
    return _str.at(pos);
  }

  Match const& at(size_type pos) const
  {
    return _str.at(pos);
  }

  bool empty() const
  {
    return _str.empty();
  }

  Regex& clear()
  {
    _str.clear();

    return *this;
  }

  Regex& shrink_to_fit()
  {
    _str.shrink_to_fit();

    return *this;
  }

  size_type size() const
  {
    return _str.size();
  }

  size_type length() const
  {
    return _str.size();
  }

  iterator begin()
  {
    return _str.begin();
  }

  const_iterator cbegin() const
  {
    return _str.cbegin();
  }

  reverse_iterator rbegin()
  {
    return _str.rbegin();
  }

  const_reverse_iterator crbegin() const
  {
    return _str.crbegin();
  }

  iterator end()
  {
    return _str.end();
  }

  const_iterator cend() const
  {
    return _str.cend();
  }

  reverse_iterator rend()
  {
    return _str.rend();
  }

  const_reverse_iterator crend() const
  {
    return _str.crend();
  }

private:

  value_type _str;
}; // class Regex

inline std::string lowercase(std::string_view const str)
{
  icu::UnicodeString ustr {icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), str.size()))};
  std::string res;
  ustr.toLower().toUTF8String(res);

  return res;
}

inline std::string uppercase(std::string_view const str)
{
  icu::UnicodeString ustr {icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), str.size()))};
  std::string res;
  ustr.toUpper().toUTF8String(res);

  return res;
}

inline std::string foldcase(std::string_view const str)
{
  icu::UnicodeString ustr {icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), str.size()))};
  std::string res;
  ustr.foldCase().toUTF8String(res);

  return res;
}

inline std::string trim(std::string_view const str)
{
  icu::UnicodeString ustr {icu::UnicodeString::fromUTF8(icu::StringPiece(str.data(), str.size()))};
  std::string res;
  ustr.trim().toUTF8String(res);

  return res;
}

inline std::int32_t compare(std::string_view const lhs, std::string_view const rhs)
{
  UErrorCode ec = U_ZERO_ERROR;

  std::unique_ptr<icu::Collator> coll {icu::Collator::createInstance(ec)};

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to create collator");
  }

  std::int32_t res {coll->compareUTF8(
    icu::StringPiece(lhs.data(), lhs.size()),
    icu::StringPiece(rhs.data(), rhs.size()),
    ec)};

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to collate text");
  }

  return res;
}

inline std::string normalize(std::string_view const str)
{
  UErrorCode ec = U_ZERO_ERROR;

  auto const norm = icu::Normalizer2::getNFKCInstance(ec);

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to create normalizer");
  }

  std::string res;
  icu::StringByteSink<std::string> bytesink (&res, str.size());

  norm->normalizeUTF8(
    0,
    icu::StringPiece(str.data(), str.size()),
    bytesink,
    NULL,
    ec);

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to normalize text");
  }

  return res;
}

inline std::string normalize_foldcase(std::string_view const str)
{
  UErrorCode ec = U_ZERO_ERROR;

  auto const norm = icu::Normalizer2::getNFKCCasefoldInstance(ec);

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to create normalizer");
  }

  std::string res;
  icu::StringByteSink<std::string> bytesink (&res, str.size());

  norm->normalizeUTF8(
    0,
    icu::StringPiece(str.data(), str.size()),
    bytesink,
    NULL,
    ec);

  if (U_FAILURE(ec))
  {
    throw std::runtime_error("failed to normalize text");
  }

  return res;
}

inline std::int32_t to_int32(std::string_view const str)
{
  if (str.empty())
  {
    return 0;
  }

  if ((str.at(0) & 0x80) == 0)
  {
    return static_cast<std::int32_t>(str.at(0));
  }
  else if ((str.at(0) & 0xE0) == 0xC0 && str.size() == 2)
  {
    return (static_cast<std::int32_t>(str[0] & 0x1F) << 6) |
      static_cast<std::int32_t>(str[1] & 0x3F);
  }
  else if ((str.at(0) & 0xF0) == 0xE0 && str.size() == 3)
  {
    return (static_cast<std::int32_t>(str[0] & 0x0F) << 12) |
      (static_cast<std::int32_t>(str[1] & 0x3F) << 6) |
      static_cast<std::int32_t>(str[2] & 0x3F);
  }
  else if ((str.at(0) & 0xF8) == 0xF0 && str.size() == 4)
  {
    return (static_cast<std::int32_t>(str[0] & 0x07) << 18) |
      (static_cast<std::int32_t>(str[1] & 0x3F) << 12) |
      (static_cast<std::int32_t>(str[2] & 0x3F) << 6) |
      static_cast<std::int32_t>(str[3] & 0x3F);
  }

  return 0;
}

inline bool is_quote(std::int32_t const ch)
{
  switch(ch)
  {
    case U'"': case U'\'': case U'«': case U'»': case U'‘': case U'’':
    case U'‚': case U'‛': case U'“': case U'”': case U'„': case U'‟':
    case U'‹': case U'›': case U'❛': case U'❜': case U'❝': case U'❞':
    case U'❟': case U'❮': case U'❯': case U'⹂': case U'「': case U'」':
    case U'『': case U'』': case U'〝': case U'〞': case U'〟': case U'＂':
      return true;

    default:
      return false;
  }
}

inline bool is_upper(std::int32_t const ch)
{
  return u_isupper(ch);
}

inline bool is_lower(std::int32_t const ch)
{
  return u_islower(ch);
}

inline bool is_punct(std::int32_t const ch)
{
  return u_ispunct(ch);
}

inline bool is_digit(std::int32_t const ch)
{
  return u_isdigit(ch);
}

inline bool is_alpha(std::int32_t const ch)
{
  return u_isalpha(ch);
}

inline bool is_alnum(std::int32_t const ch)
{
  return u_isalnum(ch);
}

inline bool is_xdigit(std::int32_t const ch)
{
  return u_isxdigit(ch);
}

inline bool is_blank(std::int32_t const ch)
{
  return u_isblank(ch);
}

inline bool is_space(std::int32_t const ch)
{
  return u_isspace(ch);
}

inline bool is_whitespace(std::int32_t const ch)
{
  return u_isWhitespace(ch);
}

inline bool is_ctrl(std::int32_t const ch)
{
  return u_iscntrl(ch);
}

inline bool is_title(std::int32_t const ch)
{
  return u_istitle(ch);
}

inline bool is_graph(std::int32_t const ch)
{
  return u_isgraph(ch);
}

inline bool is_defined(std::int32_t const ch)
{
  return u_isdefined(ch);
}

inline bool is_isoctrl(std::int32_t const ch)
{
  return u_isISOControl(ch);
}

inline bool is_print(std::int32_t const ch)
{
  return u_isprint(ch);
}

inline std::int32_t to_title(std::int32_t const ch)
{
  return u_totitle(ch);
}

inline std::int32_t to_upper(std::int32_t const ch)
{
  return u_toupper(ch);
}

inline std::int32_t to_lower(std::int32_t const ch)
{
  return u_tolower(ch);
}

} // namespace OB::Text

#endif // OB_TEXT_HH
