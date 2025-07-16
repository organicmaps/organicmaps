#pragma once

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include "base/exception.hpp"
#include "base/scope_guard.hpp"

#include "cppjansson/cppjansson.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace coding
{
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

template <typename Sink>
class SerializerJson
{
public:
  explicit SerializerJson(Sink & sink) : m_sink(sink) {}

  virtual ~SerializerJson()
  {
    std::unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(m_json.get(), 0));
    m_sink.Write(buffer.get(), strlen(buffer.get()));
  }

  template <typename T>
  void ToJsonObjectOrValue(T const & value, char const * name)
  {
    if (name != nullptr)
    {
      ToJSONObject(*m_json, name, value);
    }
    else if (json_is_array(m_json))
    {
      auto json = ToJSON(value);
      json_array_append_new(m_json.get(), json.release());
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
    NewScopeWith(base::NewJSONObject(), name, [this, &r] { r.Visit(*this); });
  }

  template <typename T, EnableIfIterable<T> * = nullptr>
  void operator()(T const & src, char const * name = nullptr)
  {
    NewScopeWith(base::NewJSONArray(), name, [this, &src]
    {
      for (auto const & v : src)
        (*this)(v);
    });
  }

  template <typename R>
  void operator()(std::unique_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(base::NewJSONObject(), name, [this, &r]
    {
      CHECK(r, ());
      r->Visit(*this);
    });
  }

  template <typename R>
  void operator()(std::shared_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(base::NewJSONObject(), name, [this, &r]
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
    NewScopeWith(base::NewJSONObject(), name, [this, &p]
    {
      (*this)(p.x, "x");
      (*this)(p.y, "y");
    });
  }

  void operator()(ms::LatLon const & ll, char const * name = nullptr)
  {
    NewScopeWith(base::NewJSONObject(), name, [this, &ll]
    {
      (*this)(ll.m_lat, "lat");
      (*this)(ll.m_lon, "lon");
    });
  }

  template <typename Key, typename Value>
  void operator()(std::pair<Key, Value> const & p, char const * name = nullptr)
  {
    NewScopeWith(base::NewJSONObject(), name, [this, &p]
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
  void NewScopeWith(base::JSONPtr json_object, char const * name, Fn && fn)
  {
    base::JSONPtr safe_json = std::move(m_json);
    m_json = std::move(json_object);

    auto rollback = [this, &safe_json, name]()
    {
      if (safe_json == nullptr)
        return;

      if (json_is_array(safe_json))
        json_array_append_new(safe_json.get(), m_json.release());
      else if (json_is_object(safe_json))
        json_object_set_new(safe_json.get(), name, m_json.release());

      m_json = std::move(safe_json);
    };
    SCOPE_GUARD(rollbackJson, rollback);

    fn();
  }

  base::JSONPtr m_json = nullptr;
  Sink & m_sink;
};

class DeserializerJson
{
public:
  using Exception = base::Json::Exception;

  template <typename Source,
            typename std::enable_if<!std::is_convertible<Source, std::string>::value, Source>::type * = nullptr>
  explicit DeserializerJson(Source & source)
  {
    auto const size = static_cast<size_t>(source.Size());
    std::string src(size, '\0');
    source.Read(static_cast<void *>(&src[0]), size);
    m_jsonObject.ParseFrom(src);
    m_json = m_jsonObject.get();
  }

  explicit DeserializerJson(std::string const & source) : m_jsonObject(source), m_json(m_jsonObject.get()) {}

  explicit DeserializerJson(char const * buffer) : m_jsonObject(buffer), m_json(m_jsonObject.get()) {}

  explicit DeserializerJson(json_t * json) : m_json(json) {}

  template <typename T>
  void FromJsonObjectOrValue(T & s, char const * name)
  {
    if (name != nullptr)
      FromJSONObject(m_json, name, s);
    else
      FromJSON(m_json, s);
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
    json_t * outerContext = SaveContext(name);

    if (!json_is_array(m_json))
      MYTHROW(base::Json::Exception, ("The field", name, "must contain a json array."));

    dest.resize(json_array_size(m_json));
    for (size_t index = 0; index < dest.size(); ++index)
    {
      json_t * context = SaveContext();
      m_json = json_array_get(context, index);
      (*this)(dest[index]);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename T, class H = std::hash<T>>
  void operator()(std::unordered_set<T, H> & dest, char const * name = nullptr)
  {
    json_t * outerContext = SaveContext(name);

    if (!json_is_array(m_json))
      MYTHROW(base::Json::Exception, ("The field", name, "must contain a json array."));

    T tmp;
    size_t const size = json_array_size(m_json);
    dest.reserve(size);
    for (size_t index = 0; index < size; ++index)
    {
      json_t * context = SaveContext();
      m_json = json_array_get(context, index);
      (*this)(tmp);
      dest.insert(tmp);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename T, size_t N>
  void operator()(std::array<T, N> & dst, char const * name = nullptr)
  {
    json_t * outerContext = SaveContext(name);

    if (!json_is_array(m_json))
      MYTHROW(base::Json::Exception, ("The field", name, "must contain a json array.", json_dumps(m_json, 0)));

    if (N != json_array_size(m_json))
    {
      MYTHROW(base::Json::Exception,
              ("The field", name, "must contain a json array of size", N, "but size is", json_array_size(m_json)));
    }

    for (size_t index = 0; index < N; ++index)
    {
      json_t * context = SaveContext();
      m_json = json_array_get(context, index);
      (*this)(dst[index]);
      RestoreContext(context);
    }

    RestoreContext(outerContext);
  }

  template <typename Key, typename T>
  void operator()(std::map<Key, T> & dst, char const * name = nullptr)
  {
    json_t * outerContext = SaveContext(name);

    if (!json_is_array(m_json))
      MYTHROW(base::Json::Exception, ("The field", name, "must contain a json array.", json_dumps(m_json, 0)));

    size_t const size = json_array_size(m_json);
    for (size_t index = 0; index < size; ++index)
    {
      json_t * context = SaveContext();
      m_json = json_array_get(context, index);
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
    json_t * outerContext = SaveContext(name);
    (*this)(dst.first, "key");
    (*this)(dst.second, "value");
    RestoreContext(outerContext);
  }

  template <typename R, EnableIfNotEnum<R> * = nullptr, EnableIfNotVectorOrDeque<R> * = nullptr>
  void operator()(R & r, char const * name = nullptr)
  {
    json_t * context = SaveContext(name);
    r.Visit(*this);
    RestoreContext(context);
  }

  template <typename R>
  void operator()(std::unique_ptr<R> & r, char const * name = nullptr)
  {
    json_t * context = SaveContext(name);
    if (!r)
      r = std::make_unique<R>();
    r->Visit(*this);
    RestoreContext(context);
  }

  template <typename R>
  void operator()(std::shared_ptr<R> & r, char const * name = nullptr)
  {
    json_t * context = SaveContext(name);
    if (!r)
      r = std::make_shared<R>();
    r->Visit(*this);
    RestoreContext(context);
  }

  void operator()(std::chrono::system_clock::time_point & dst, char const * name = nullptr)
  {
    uint64_t t = 0;
    FromJSONObject(m_json, name, t);

    std::chrono::system_clock::time_point::duration d(t);

    dst = std::chrono::system_clock::time_point(d);
  }

  template <typename T, EnableIfEnum<T> * = nullptr>
  void operator()(T & t, char const * name = nullptr)
  {
    using UnderlyingType = std::underlying_type_t<T>;
    UnderlyingType res;
    FromJSONObject(m_json, name, res);
    t = static_cast<T>(res);
  }

  void operator()(m2::PointD & p, char const * name = nullptr)
  {
    json_t * outerContext = SaveContext(name);
    (*this)(p.x, "x");
    (*this)(p.y, "y");
    RestoreContext(outerContext);
  }

  void operator()(ms::LatLon & ll, char const * name = nullptr)
  {
    json_t * outerContext = SaveContext(name);
    (*this)(ll.m_lat, "lat");
    (*this)(ll.m_lon, "lon");
    RestoreContext(outerContext);
  }

  template <typename Optional>
  void operator()(Optional & opt, Optional const & defaultValue, char const * name = nullptr)
  {
    auto json = base::GetJSONOptionalField(m_json, name);
    if (!json)
    {
      opt = defaultValue;
      return;
    }

    (*this)(opt, name);
  }

protected:
  json_t * SaveContext(char const * name = nullptr)
  {
    json_t * context = m_json;
    if (name)
      m_json = base::GetJSONObligatoryField(context, name);
    return context;
  }

  void RestoreContext(json_t * context)
  {
    if (context)
      m_json = context;
  }

  base::Json m_jsonObject;
  json_t * m_json = nullptr;
};
}  // namespace coding
