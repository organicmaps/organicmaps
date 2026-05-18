#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/exception.hpp"
#include "base/scope_guard.hpp"

#include <glaze/json.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace coding
{
using JsonValue = glz::generic_u64;

DECLARE_EXCEPTION(JsonException, RootException);

namespace traits
{
namespace impl
{
template <typename T>
auto is_iterable_checker(int)
    -> decltype(std::begin(std::declval<T>()), std::end(std::declval<T>()),
                ++std::declval<decltype(std::begin(std::declval<T>())) &>(), std::true_type{});

template <typename T>
std::false_type is_iterable_checker(...);

template <typename T>
auto is_dynamic_sequence_checker(int)
    -> decltype(std::declval<T &>().resize(0), std::declval<T &>()[0], std::true_type{});

template <typename T>
std::false_type is_dynamic_sequence_checker(...);
}  // namespace impl

template <typename T>
using is_iterable = decltype(impl::is_iterable_checker<T>(0));

template <typename T>
using is_dynamic_sequence = decltype(impl::is_dynamic_sequence_checker<T>(0));
}  // namespace traits

template <typename T>
using EnableIfIterable = std::enable_if_t<traits::is_iterable<T>::value>;

template <typename T>
using EnableIfNotIterable = std::enable_if_t<!traits::is_iterable<T>::value>;

template <typename T>
using EnableIfEnum = std::enable_if_t<std::is_enum<T>::value>;

template <typename T>
using EnableIfNotEnum = std::enable_if_t<!std::is_enum<T>::value>;

template <typename T>
using EnableIfVectorOrDeque =
    std::enable_if_t<traits::is_dynamic_sequence<T>::value && !std::is_same<std::string, T>::value>;

template <typename T>
using EnableIfNotVectorOrDeque = std::enable_if_t<!traits::is_dynamic_sequence<T>::value>;

namespace detail
{
inline JsonValue MakeJSONObject()
{
  JsonValue value;
  value.data = JsonValue::object_t{};
  return value;
}

inline JsonValue MakeJSONArray()
{
  JsonValue value;
  value.data = JsonValue::array_t{};
  return value;
}

inline void ThrowJsonError(std::string const & message)
{
  MYTHROW(JsonException, (message));
}

inline JsonValue const * GetOptionalField(JsonValue const * root, char const * field)
{
  if (root == nullptr || !root->is_object())
    MYTHROW(JsonException, ("Bad json object while parsing", field));

  auto const & object = root->get_object();
  auto const it = object.find(field);
  if (it == object.end())
    return nullptr;
  return &it->second;
}

inline JsonValue * GetOptionalField(JsonValue * root, char const * field)
{
  return const_cast<JsonValue *>(GetOptionalField(const_cast<JsonValue const *>(root), field));
}

inline JsonValue const * GetObligatoryField(JsonValue const * root, char const * field)
{
  JsonValue const * value = GetOptionalField(root, field);
  if (value == nullptr)
    MYTHROW(JsonException, ("Obligatory field", field, "is absent."));
  return value;
}

inline JsonValue * GetObligatoryField(JsonValue * root, char const * field)
{
  return const_cast<JsonValue *>(GetObligatoryField(const_cast<JsonValue const *>(root), field));
}

template <typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
void ReadPrimitive(JsonValue const & root, T & result)
{
  if (!root.is_number())
    MYTHROW(JsonException, ("Object must contain a json number."));

  result = root.as<T>();
}

inline void ReadPrimitive(JsonValue const & root, double & result)
{
  if (!root.is_number())
    MYTHROW(JsonException, ("Object must contain a json number."));

  result = root.as<double>();
}

inline void ReadPrimitive(JsonValue const & root, bool & result)
{
  if (!root.is_boolean())
    MYTHROW(JsonException, ("Object must contain a boolean value."));

  result = root.get_boolean();
}

inline void ReadPrimitive(JsonValue const & root, std::string & result)
{
  if (!root.is_string())
    MYTHROW(JsonException, ("The field must contain a json string."));

  result = root.get_string();
}

template <typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
JsonValue ToJsonValue(T value)
{
  return JsonValue(value);
}

inline JsonValue ToJsonValue(double value)
{
  return JsonValue(value);
}

inline JsonValue ToJsonValue(bool value)
{
  return JsonValue(value);
}

inline JsonValue ToJsonValue(std::string const & value)
{
  return JsonValue(value);
}

inline JsonValue ToJsonValue(char const * value)
{
  return JsonValue(value);
}
}  // namespace detail

template <typename Sink>
class SerializerJson
{
public:
  explicit SerializerJson(Sink & sink) : m_sink(sink) {}

  virtual ~SerializerJson() noexcept(false)
  {
    std::string buffer;
    if (auto const error = glz::write_json(m_json, buffer); error)
      MYTHROW(JsonException, ("Failed to write JSON", glz::format_error(error)));

    m_sink.Write(buffer.data(), buffer.size());
  }

  template <typename T>
  void ToJsonObjectOrValue(T const & value, char const * name)
  {
    JsonValue json = detail::ToJsonValue(value);
    if (name != nullptr)
    {
      if (!m_json.is_object())
        MYTHROW(JsonException, ("Current JSON context must be an object."));
      m_json[name] = std::move(json);
    }
    else if (m_json.is_array())
    {
      m_json.get_array().push_back(std::move(json));
    }
    else if (m_json.is_null())
    {
      m_json = std::move(json);
    }
    else
    {
      ASSERT(false, ("Unsupported JSON structure"));
    }
  }

  void operator()(bool const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(uint8_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(uint32_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(uint64_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(int8_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(int32_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(int64_t const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(double const d, char const * name = nullptr) { ToJsonObjectOrValue(d, name); }
  void operator()(std::string const & s, char const * name = nullptr) { ToJsonObjectOrValue(s, name); }

  template <typename R, EnableIfNotIterable<R> * = nullptr, EnableIfNotEnum<R> * = nullptr>
  void operator()(R const & r, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &r] { r.Visit(*this); });
  }

  template <typename T, EnableIfIterable<T> * = nullptr>
  void operator()(T const & src, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONArray(), name, [this, &src]
    {
      for (auto const & v : src)
        (*this)(v);
    });
  }

  template <typename R>
  void operator()(std::unique_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &r]
    {
      CHECK(r, ());
      r->Visit(*this);
    });
  }

  template <typename R>
  void operator()(std::shared_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &r]
    {
      CHECK(r, ());
      r->Visit(*this);
    });
  }

  void operator()(std::chrono::system_clock::time_point const & t, char const * name = nullptr)
  {
    (*this)(static_cast<uint64_t>(t.time_since_epoch().count()), name);
  }

  template <typename T, EnableIfEnum<T> * = nullptr>
  void operator()(T const & t, char const * name = nullptr)
  {
    (*this)(static_cast<std::underlying_type_t<T>>(t), name);
  }

  void operator()(m2::PointD const & p, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &p]
    {
      (*this)(p.x, "x");
      (*this)(p.y, "y");
    });
  }

  void operator()(ms::LatLon const & ll, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &ll]
    {
      (*this)(ll.m_lat, "lat");
      (*this)(ll.m_lon, "lon");
    });
  }

  template <typename Key, typename Value>
  void operator()(std::pair<Key, Value> const & p, char const * name = nullptr)
  {
    NewScopeWith(detail::MakeJSONObject(), name, [this, &p]
    {
      (*this)(p.first, "key");
      (*this)(p.second, "value");
    });
  }

  template <typename Optional>
  void operator()(Optional const & opt, Optional const &, char const * name = nullptr)
  {
    (*this)(opt, name);
  }

protected:
  template <typename Fn>
  void NewScopeWith(JsonValue jsonObject, char const * name, Fn && fn)
  {
    JsonValue previous = std::move(m_json);
    m_json = std::move(jsonObject);

    auto rollback = [this, &previous, name]()
    {
      if (previous.is_null())
        return;

      if (previous.is_array())
        previous.get_array().push_back(std::move(m_json));
      else if (previous.is_object())
        previous[name] = std::move(m_json);

      m_json = std::move(previous);
    };
    SCOPE_GUARD(rollbackJson, rollback);

    fn();
  }

  JsonValue m_json;
  Sink & m_sink;
};

class DeserializerJson
{
public:
  using Exception = JsonException;

  template <typename Source,
            typename std::enable_if<!std::is_convertible<Source, std::string>::value, Source>::type * = nullptr>
  explicit DeserializerJson(Source & source)
  {
    auto const size = static_cast<size_t>(source.Size());
    std::string src(size, '\0');
    source.Read(static_cast<void *>(&src[0]), size);
    ParseFrom(src);
  }

  explicit DeserializerJson(std::string const & source) { ParseFrom(source); }

  explicit DeserializerJson(char const * buffer) { ParseFrom(buffer); }

  template <typename T>
  void FromJsonObjectOrValue(T & s, char const * name)
  {
    if (name != nullptr)
      detail::ReadPrimitive(*detail::GetObligatoryField(m_json, name), s);
    else
      detail::ReadPrimitive(*m_json, s);
  }

  void operator()(bool & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(uint8_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(uint32_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(uint64_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(int8_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(int32_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(int64_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(double & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(std::string & s, char const * name = nullptr) { FromJsonObjectOrValue(s, name); }

  template <typename T, EnableIfVectorOrDeque<T> * = nullptr>
  void operator()(T & dest, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    auto * array = m_json->get_if<JsonValue::array_t>();
    if (array == nullptr)
      MYTHROW(JsonException, ("The field", name, "must contain a json array."));

    dest.resize(array->size());
    for (size_t index = 0; index < dest.size(); ++index)
    {
      JsonValue * context = SaveContext();
      m_json = &(*array)[index];
      (*this)(dest[index]);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename T, class H = std::hash<T>>
  void operator()(std::unordered_set<T, H> & dest, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    auto * array = m_json->get_if<JsonValue::array_t>();
    if (array == nullptr)
      MYTHROW(JsonException, ("The field", name, "must contain a json array."));

    dest.reserve(array->size());
    for (auto const & item : *array)
    {
      T tmp;
      JsonValue * context = SaveContext();
      m_json = const_cast<JsonValue *>(&item);
      (*this)(tmp);
      dest.insert(tmp);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename T, size_t N>
  void operator()(std::array<T, N> & dst, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    auto * array = m_json->get_if<JsonValue::array_t>();
    if (array == nullptr)
      MYTHROW(JsonException, ("The field", name, "must contain a json array."));

    if (N != array->size())
      MYTHROW(JsonException, ("The field", name, "must contain a json array of size", N, "but size is", array->size()));

    for (size_t index = 0; index < N; ++index)
    {
      JsonValue * context = SaveContext();
      m_json = const_cast<JsonValue *>(&(*array)[index]);
      (*this)(dst[index]);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename Key, typename T>
  void operator()(std::map<Key, T> & dst, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    auto * array = m_json->get_if<JsonValue::array_t>();
    if (array == nullptr)
      MYTHROW(JsonException, ("The field", name, "must contain a json array."));

    for (auto const & item : *array)
    {
      JsonValue * context = SaveContext();
      m_json = const_cast<JsonValue *>(&item);
      std::pair<Key, T> tmp;
      (*this)(tmp);
      dst.insert(tmp);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename Key, typename Value>
  void operator()(std::pair<Key, Value> & dst, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    (*this)(dst.first, "key");
    (*this)(dst.second, "value");
    RestoreContext(outerContext);
  }

  template <typename R, EnableIfNotEnum<R> * = nullptr, EnableIfNotVectorOrDeque<R> * = nullptr>
  void operator()(R & r, char const * name = nullptr)
  {
    JsonValue * context = SaveContext(name);
    r.Visit(*this);
    RestoreContext(context);
  }

  template <typename R>
  void operator()(std::unique_ptr<R> & r, char const * name = nullptr)
  {
    JsonValue * context = SaveContext(name);
    if (!r)
      r = std::make_unique<R>();
    r->Visit(*this);
    RestoreContext(context);
  }

  template <typename R>
  void operator()(std::shared_ptr<R> & r, char const * name = nullptr)
  {
    JsonValue * context = SaveContext(name);
    if (!r)
      r = std::make_shared<R>();
    r->Visit(*this);
    RestoreContext(context);
  }

  void operator()(std::chrono::system_clock::time_point & dst, char const * name = nullptr)
  {
    uint64_t t = 0;
    FromJsonObjectOrValue(t, name);

    std::chrono::system_clock::time_point::duration d(t);
    dst = std::chrono::system_clock::time_point(d);
  }

  template <typename T, EnableIfEnum<T> * = nullptr>
  void operator()(T & t, char const * name = nullptr)
  {
    using UnderlyingType = std::underlying_type_t<T>;
    UnderlyingType res;
    FromJsonObjectOrValue(res, name);
    t = static_cast<T>(res);
  }

  void operator()(m2::PointD & p, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    (*this)(p.x, "x");
    (*this)(p.y, "y");
    RestoreContext(outerContext);
  }

  void operator()(ms::LatLon & ll, char const * name = nullptr)
  {
    JsonValue * outerContext = SaveContext(name);
    (*this)(ll.m_lat, "lat");
    (*this)(ll.m_lon, "lon");
    RestoreContext(outerContext);
  }

  template <typename Optional>
  void operator()(Optional & opt, Optional const & defaultValue, char const * name = nullptr)
  {
    if (detail::GetOptionalField(m_json, name) == nullptr)
    {
      opt = defaultValue;
      return;
    }

    (*this)(opt, name);
  }

protected:
  void ParseFrom(std::string const & source)
  {
    if (auto const error = glz::read_json(m_jsonObject, source); error)
      MYTHROW(JsonException, (glz::format_error(error, source)));

    m_json = &m_jsonObject;
  }

  JsonValue * SaveContext(char const * name = nullptr)
  {
    JsonValue * context = m_json;
    if (name)
      m_json = detail::GetObligatoryField(context, name);
    return context;
  }

  void RestoreContext(JsonValue * context)
  {
    if (context)
      m_json = context;
  }

  JsonValue m_jsonObject;
  JsonValue * m_json = nullptr;
};
}  // namespace coding
