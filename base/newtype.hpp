#pragma once

#include <functional>
#include <string>
#include <type_traits>

namespace base
{
namespace impl
{
template <typename From, typename To>
using IsConvertibleGuard = std::enable_if_t<std::is_convertible<From, To>::value> *;
}  // namespace impl

/// Creates a typesafe alias to a given numeric Type.
template <typename Type, typename Tag, typename Hasher = std::hash<Type>>
class NewType
{
  static_assert(std::is_integral<Type>::value || std::is_floating_point<Type>::value,
                "NewType can be used only with integral and floating point type.");

public:
  using RepType = Type;

  template <typename V, impl::IsConvertibleGuard<V, Type> = nullptr>
  constexpr explicit NewType(V const & v) : m_value(v)
  {}

  constexpr NewType() = default;

  template <typename V, impl::IsConvertibleGuard<V, Type> = nullptr>
  NewType & Set(V const & v)
  {
    m_value = static_cast<Type>(v);
    return *this;
  }

  Type const & Get() const { return m_value; }
  Type & Get() { return m_value; }

  NewType & operator++()
  {
    ++m_value;
    return *this;
  }

  NewType const operator++(int)
  {
    auto const copy = *this;
    ++m_value;
    return copy;
  }

  NewType & operator--()
  {
    --m_value;
    return *this;
  }

  NewType const operator--(int)
  {
    auto const copy = *this;
    --m_value;
    return copy;
  }

  NewType & operator+=(NewType const & v)
  {
    m_value += v.m_value;
    return *this;
  }

  NewType & operator-=(NewType const & v)
  {
    m_value -= v.m_value;
    return *this;
  }

  NewType & operator*=(NewType const & v)
  {
    m_value *= v.m_value;
    return *this;
  }

  NewType & operator/=(NewType const & v)
  {
    m_value /= v.m_value;
    return *this;
  }

  NewType & operator%=(NewType const & v)
  {
    m_value %= v.m_value;
    return *this;
  }

  NewType & operator^=(NewType const & v)
  {
    m_value ^= v.m_value;
    return *this;
  }

  NewType & operator|=(NewType const & v)
  {
    m_value |= v.m_value;
    return *this;
  }

  NewType & operator&=(NewType const & v)
  {
    m_value &= v.m_value;
    return *this;
  }

  // TODO(mgsergio): Is it meaningful for a newtype to have << operator ?
  // NewType & operator<<=(NewType<V, VTag> const & v)
  // NewType & operator>>=(NewType<V, VTag> const & v)

  bool operator==(NewType const & o) const { return m_value == o.m_value; }
  bool operator!=(NewType const & o) const { return !(m_value == o.m_value); }
  bool operator<(NewType const & o) const { return m_value < o.m_value; }
  bool operator>(NewType const & o) const { return m_value > o.m_value; }
  bool operator<=(NewType const & o) const { return !(m_value > o.m_value); }
  bool operator>=(NewType const & o) const { return !(m_value < o.m_value); }
  NewType operator+(NewType const & o) const { return NewType(m_value + o.m_value); }
  NewType operator-(NewType const & o) const { return NewType(m_value - o.m_value); }
  NewType operator*(NewType const & o) const { return NewType(m_value * o.m_value); }
  NewType operator/(NewType const & o) const { return NewType(m_value / o.m_value); }
  NewType operator%(NewType const & o) const { return NewType(m_value % o.m_value); }
  NewType operator^(NewType const & o) const { return NewType(m_value ^ o.m_value); }
  NewType operator|(NewType const & o) const { return NewType(m_value | o.m_value); }
  NewType operator&(NewType const & o) const { return NewType(m_value & o.m_value); }

  struct Hash
  {
    size_t operator()(NewType const & v) const
    {
      Hasher h;
      return h(v.Get());
    }
  };

private:
  Type m_value;
};

namespace newtype_default_output
{
template <typename Type, typename Tag>
std::string SimpleDebugPrint(NewType<Type, Tag> const & nt)
{
  return ::DebugPrint(nt.Get());
}
}  // namespace newtype_default_output
}  // namespace base

#define NEWTYPE(REPR, NAME)                    \
  struct NAME##_tag;                           \
  using NAME = base::NewType<REPR, NAME##_tag>

#define NEWTYPE_SIMPLE_OUTPUT(NAME)                                     \
  inline std::string DebugPrint(NAME const & nt)                        \
  {                                                                     \
    return base::newtype_default_output::SimpleDebugPrint(nt);          \
  }                                                                     \
  inline std::ostream & operator<<(std::ostream & ost, NAME const & nt) \
  {                                                                     \
    return ost << base::newtype_default_output::SimpleDebugPrint(nt);   \
  }
