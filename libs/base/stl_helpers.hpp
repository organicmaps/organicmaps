#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

namespace base
{
using StringIL = std::initializer_list<char const *>;

namespace impl
{
// When isField is true, following functors operate on a
// pointers-to-field.  Otherwise, they operate on a
// pointers-to-const-method.
template <bool isField, typename T, typename C>
struct Less;

template <typename T, typename C>
struct Less<true, T, C>
{
  explicit Less(T C::* p) : m_p(p) {}

  bool operator()(C const & lhs, C const & rhs) const { return lhs.*m_p < rhs.*m_p; }

  bool operator()(C const * const lhs, C const * const rhs) const { return lhs->*m_p < rhs->*m_p; }

  T C::* m_p;
};

template <typename T, typename C>
struct Less<false, T, C>
{
  explicit Less(T (C::*p)() const) : m_p(p) {}

  bool operator()(C const & lhs, C const & rhs) const { return (lhs.*m_p)() < (rhs.*m_p)(); }

  bool operator()(C const * const lhs, C const * const rhs) const { return (lhs->*m_p)() < (rhs->*m_p)(); }

  T (C::*m_p)() const;
};

template <bool isField, typename T, typename C>
struct Equals;

template <typename T, typename C>
struct Equals<true, T, C>
{
  explicit Equals(T C::* p) : m_p(p) {}

  bool operator()(C const & lhs, C const & rhs) const { return lhs.*m_p == rhs.*m_p; }

  bool operator()(C const * const lhs, C const * const rhs) const { return lhs->*m_p == rhs->*m_p; }

  T C::* m_p;
};

template <typename T, typename C>
struct Equals<false, T, C>
{
  explicit Equals(T (C::*p)() const) : m_p(p) {}

  bool operator()(C const & lhs, C const & rhs) const { return (lhs.*m_p)() == (rhs.*m_p)(); }

  bool operator()(C const * const lhs, C const * const rhs) const { return (lhs->*m_p)() == (rhs->*m_p)(); }

  T (C::*m_p)() const;
};
}  // namespace impl

// Sorts and removes duplicate entries from |c|.
template <typename Cont>
void SortUnique(Cont & c)
{
  std::sort(c.begin(), c.end());
  c.erase(std::unique(c.begin(), c.end()), c.end());
}

template <typename Cont, typename Equals>
void Unique(Cont & c, Equals && equals)
{
  c.erase(std::unique(c.begin(), c.end(), std::forward<Equals>(equals)), c.end());
}

template <typename Cont, typename Less, typename Equals>
void SortUnique(Cont & c, Less && less, Equals && equals)
{
  std::sort(c.begin(), c.end(), std::forward<Less>(less));
  Unique(c, equals);
}

template <typename Cont, typename Fn>
void EraseIf(Cont & c, Fn && fn)
{
  c.erase(std::remove_if(c.begin(), c.end(), std::forward<Fn>(fn)), c.end());
}

template <typename Cont, typename Fn>
bool AllOf(Cont const & c, Fn && fn)
{
  return std::all_of(std::cbegin(c), std::cend(c), std::forward<Fn>(fn));
}

template <typename Cont, typename Fn>
bool AnyOf(Cont const & c, Fn && fn)
{
  return std::any_of(std::cbegin(c), std::cend(c), std::forward<Fn>(fn));
}

template <typename Cont, typename OutIt, typename Fn>
decltype(auto) Transform(Cont const & c, OutIt it, Fn && fn)
{
  return std::transform(std::cbegin(c), std::cend(c), it, std::forward<Fn>(fn));
}

template <typename Cont, typename Fn>
decltype(auto) FindIf(Cont const & c, Fn && fn)
{
  return std::find_if(std::cbegin(c), std::cend(c), std::forward<Fn>(fn));
}

template <typename Cont, typename T>
bool IsExist(Cont const & c, T const & t)
{
  auto const end = std::cend(c);
  return std::find(std::cbegin(c), end, t) != end;
}

template <class MapT, class K, class V>
auto EmplaceOrAssign(MapT & theMap, K && k, V && v)
{
  auto it = theMap.lower_bound(k);
  if (it != theMap.end() && k == it->first)
  {
    it->second = std::forward<V>(v);
    return std::make_pair(it, false);
  }
  return std::make_pair(theMap.emplace_hint(it, std::forward<K>(k), std::forward<V>(v)), true);
}

// Creates a comparer being able to compare two instances of class C
// (given by reference or pointer) by a field or const method of C.
// For example, to create comparer that is able to compare pairs of
// ints by second component, it's enough to call LessBy(&pair<int,
// int>::second).
template <typename T, typename C>
impl::Less<true, T, C> LessBy(T C::* p)
{
  return impl::Less<true, T, C>(p);
}

template <typename T, typename C>
impl::Less<false, T, C> LessBy(T (C::*p)() const)
{
  return impl::Less<false, T, C>(p);
}

template <typename T, typename C>
impl::Equals<true, T, C> EqualsBy(T C::* p)
{
  return impl::Equals<true, T, C>(p);
}

template <typename T, typename C>
impl::Equals<false, T, C> EqualsBy(T (C::*p)() const)
{
  return impl::Equals<false, T, C>(p);
}

template <typename T>
std::underlying_type_t<T> constexpr Underlying(T value)
{
  return static_cast<std::underlying_type_t<T>>(value);
}

// Short alias like Enum to Integral.
template <typename T>
std::underlying_type_t<T> constexpr E2I(T value)
{
  return Underlying(value);
}

template <typename Container>
class BackInsertFunctor
{
public:
  explicit BackInsertFunctor(Container & container) : m_Container(container) {}
  template <class T>
  void operator()(T && t) const
  {
    m_Container.push_back(std::forward<T>(t));
  }

private:
  Container & m_Container;
};

template <typename Container>
BackInsertFunctor<Container> MakeBackInsertFunctor(Container & container)
{
  return BackInsertFunctor<Container>(container);
}

template <typename Container>
class InsertFunctor
{
public:
  explicit InsertFunctor(Container & container) : m_Container(container) {}
  void operator()(typename Container::value_type const & t) const { m_Container.insert(end(m_Container), t); }

private:
  Container & m_Container;
};

template <typename Container>
InsertFunctor<Container> MakeInsertFunctor(Container & container)
{
  return InsertFunctor<Container>(container);
}

template <typename Iter, typename Compare>
bool IsSortedAndUnique(Iter beg, Iter end, Compare comp)
{
  if (beg == end)
    return true;
  Iter prev = beg;
  for (++beg; beg != end; ++beg, ++prev)
    if (!comp(*prev, *beg))
      return false;
  return true;
}

template <typename ContT>
bool IsSortedAndUnique(ContT const & cont)
{
  return IsSortedAndUnique(cont.begin(), cont.end(), std::less{});
}

template <typename Iter, typename Compare>
Iter RemoveIfKeepValid(Iter beg, Iter end, Compare comp)
{
  while (beg != end)
  {
    if (comp(*beg))
    {
      while (beg != --end)
      {
        if (!comp(*end))
        {
          std::swap(*beg, *end);
          ++beg;
          break;
        }
      }
    }
    else
      ++beg;
  }

  return end;
}

struct DeleteFunctor
{
  template <typename T>
  void operator()(T const * p) const
  {
    delete p;
  }
};

struct IdFunctor
{
  template <typename T>
  T operator()(T const & x) const
  {
    return x;
  }
};

template <typename T>
struct EqualFunctor
{
  T const & m_t;
  explicit EqualFunctor(T const & t) : m_t(t) {}
  bool operator()(T const & t) const { return (t == m_t); }
};

template <typename Iter>
Iter NextIterInCycle(Iter it, Iter beg, Iter end)
{
  if (++it == end)
    return beg;
  return it;
}

template <typename Iter>
Iter PrevIterInCycle(Iter it, Iter beg, Iter end)
{
  if (it == beg)
    it = end;
  return --it;
}

struct RetrieveSecond
{
  template <typename T>
  typename T::second_type const & operator()(T const & pair) const
  {
    return pair.second;
  }
};

template <std::ranges::random_access_range Container>
  requires std::totally_ordered<std::ranges::range_value_t<Container>>
consteval bool HasUniqueElements(Container container)
{
  std::sort(container.begin(), container.end());
  return std::adjacent_find(container.begin(), container.end()) == container.end();
}
}  // namespace base
