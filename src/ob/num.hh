#ifndef OB_NUM_HH
#define OB_NUM_HH

#include <cstdint>
#include <cstddef>

#include <limits>
#include <string>
#include <sstream>
#include <iostream>

namespace OB
{

template<typename T>
class basic_num
{
public:

  basic_num(T val) noexcept :
    _val {val}
  {
  }

  basic_num(T val, T min, T max) noexcept :
    _val {val},
    _min {min},
    _max {max}
  {
  }

  basic_num() = default;

  basic_num(basic_num const&) = default;
  basic_num(basic_num&&) = default;

  basic_num& operator=(basic_num const&) = default;
  basic_num& operator=(basic_num&&) = default;

  basic_num& operator=(T const val)
  {
    validate(val);

    return *this;
  }

  ~basic_num() = default;

  template<typename V>
  friend std::ostream& operator<<(std::ostream& os, basic_num<V> const& obj);
  template<typename V>
  friend std::istream& operator>>(std::istream& is, basic_num<V>& obj);

  T get() const
  {
    return _val;
  }

  template<typename V, std::enable_if_t<std::is_integral_v<V>, int> = 0>
  V get() const
  {
    return static_cast<V>(_val);
  }

  operator bool() const
  {
    return static_cast<bool>(_val);
  }

  operator std::string() const
  {
    return str();
  }

  bool operator!() const
  {
    return ! static_cast<bool>(*this);
  }

  basic_num<T>& operator++()
  {
    *this += 1;

    return *this;
  }

  basic_num<T>& operator--()
  {
    *this -= 1;

    return *this;
  }

  basic_num<T> operator++(int)
  {
    basic_num<T> tmp {*this};
    operator++();

    return tmp;
  }

  basic_num<T> operator--(int)
  {
    basic_num<T> tmp {*this};
    operator--();

    return tmp;
  }

  template<typename V>
  inline basic_num<V> operator+(basic_num<V> const& obj)
  {
    *this += obj;

    return *this;
  }

  template<typename V>
  inline basic_num<V> operator-(basic_num<V> const& obj)
  {
    *this -= obj;

    return *this;
  }

  template<typename V>
  inline basic_num<V> operator*(V const& obj)
  {
    *this *= obj;

    return *this;
  }

  template<typename V>
  inline basic_num<V> operator/(V const& obj)
  {
    *this /= obj;

    return *this;
  }

  template<typename V>
  inline basic_num<V> operator%(V const& obj)
  {
    *this %= obj;

    return *this;
  }

  template<typename V>
  inline bool operator<(V const& obj) const
  {
    return _val < obj;
  }

  template<typename V>
  inline bool operator>(V const& obj) const
  {
    return obj < *this;
  }

  template<typename V>
  inline bool operator<=(V const& obj) const
  {
    return !(*this > obj);
  }

  template<typename V>
  inline bool operator>=(V const& obj) const
  {
    return !(*this < obj);
  }

  template<typename V>
  inline bool operator==(V const& obj) const
  {
    return _val == obj;
  }

  template<typename V>
  inline bool operator!=(V const& obj) const
  {
    return !(*this == obj);
  }


  basic_num<T>& operator+=(basic_num<T> const& obj)
  {
    validate(_val + obj._val);

    return *this;
  }

  basic_num<T>& operator-=(basic_num<T> const& obj)
  {
    validate(_val - obj._val);

    return *this;
  }

  basic_num<T>& operator*=(basic_num<T> const& obj)
  {
    validate(_val * obj._val);

    return *this;
  }

  basic_num<T>& operator/=(basic_num<T> const& obj)
  {
    validate(_val * obj._val);

    return *this;
  }

  basic_num<T>& operator%=(basic_num<T> const& obj)
  {
    validate(_val % obj._val);

    return *this;
  }

  std::string str() const
  {
    return std::to_string(_val);
  }

  T val() const
  {
    return _val;
  }

  T min() const
  {
    return _min;
  }

  basic_num<T>& min(T const& val)
  {
    _min = val;

    return *this;
  }

  T max() const
  {
    return _max;
  }

  basic_num<T>& max(T const& val)
  {
    _max = val;

    return *this;
  }

private:

  void validate(T const val)
  {
    if (static_cast<int>(val) < static_cast<int>(_min))
    {
      _val = _min;
    }
    else if (static_cast<int>(val) > static_cast<int>(_max))
    {
      _val = _max;
    }
    else
    {
      _val = val;
    }
  }

  T _val {0};
  T _min {std::numeric_limits<T>::lowest()};
  T _max {std::numeric_limits<T>::max()};
}; // class basic_num<T>

template<typename T>
inline std::ostream& operator<<(std::ostream& os, basic_num<T> const& obj)
{
  os << obj._val;

  return os;
}

template<typename T>
inline std::istream& operator>>(std::istream& is, basic_num<T> const& obj)
{
  is >> obj._val;

  return is;
}

using num = basic_num<int>;

using num_float = basic_num<float>;
using num_double = basic_num<double>;

using num_size = basic_num<std::size_t>;

using num_8 = basic_num<std::int8_t>;
using num_16 = basic_num<std::int16_t>;
using num_32 = basic_num<std::int32_t>;
using num_64 = basic_num<std::int64_t>;

using num_u8 = basic_num<std::uint8_t>;
using num_u16 = basic_num<std::uint16_t>;
using num_u32 = basic_num<std::uint32_t>;
using num_u64 = basic_num<std::uint64_t>;

} // namespace OB

#endif // OB_NUM_HH
