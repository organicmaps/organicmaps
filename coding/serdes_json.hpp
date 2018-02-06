#pragma once

#include "base/exception.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include "3party/jansson/myjansson.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace coding
{
template<typename Sink>
class SerializerJson
{
public:
  explicit SerializerJson(Sink & sink) : m_sink(sink) {}

  virtual ~SerializerJson()
  {
    std::unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(m_json.get(), 0));
    m_sink.Write(buffer.get(), strlen(buffer.get()));
  }

  void operator()(bool const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint8_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint32_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint64_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(int64_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(double const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(std::string const & s, char const * name = nullptr)
  {
    ToJSONObject(*m_json, name, s);
  }

  template <typename T>
  void operator()(std::vector<T> const & vs, char const * name = nullptr)
  {
    NewScopeWith(my::NewJSONArray(), name, [this, &vs] {
      for (auto const & v : vs)
        (*this)(v);
    });
  }

  template <typename R>
  void operator()(R const & r, char const * name = nullptr)
  {
    NewScopeWith(my::NewJSONObject(), name, [this, &r] { r.Visit(*this); });
  }

  template <typename R>
  void operator()(std::unique_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(my::NewJSONObject(), name, [this, &r] {
      CHECK(r, ());
      r->Visit(*this);
    });
  }

  template <typename R>
  void operator()(std::shared_ptr<R> const & r, char const * name = nullptr)
  {
    NewScopeWith(my::NewJSONObject(), name, [this, &r] {
      CHECK(r, ());
      r->Visit(*this);
    });
  }

protected:
  template <typename Fn>
  void NewScopeWith(my::JSONPtr json_object, char const * name, Fn && fn)
  {
    my::JSONPtr safe_json = std::move(m_json);
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
    MY_SCOPE_GUARD(rollbackJson, rollback);

    fn();
  }

  my::JSONPtr m_json = nullptr;
  Sink & m_sink;
};

class DeserializerJson
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  template <typename Source,
            typename std::enable_if<!std::is_convertible<Source, std::string>::value,
                                    Source>::type * = nullptr>
  explicit DeserializerJson(Source & source)
  {
    std::string src(source.Size(), '\0');
    source.Read(static_cast<void *>(&src[0]), source.Size());
    m_jsonObject.ParseFrom(src);
    m_json = m_jsonObject.get();
  }

  explicit DeserializerJson(std::string const & source)
    : m_jsonObject(source), m_json(m_jsonObject.get())
  {
  }

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
  void operator()(int64_t & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(double & d, char const * name = nullptr) { FromJsonObjectOrValue(d, name); }
  void operator()(std::string & s, char const * name = nullptr) { FromJsonObjectOrValue(s, name); }

  template <typename T>
  void operator()(std::vector<T> & vs, char const * name = nullptr)
  {
    json_t * context = SaveContext(name);

    if (!json_is_array(m_json))
      MYTHROW(my::Json::Exception, ("The field", name, "must contain a json array."));

    vs.resize(json_array_size(m_json));
    for (size_t index = 0; index < vs.size(); ++index)
    {
      json_t * context = SaveContext();
      m_json = json_array_get(context, index);
      (*this)(vs[index]);
      RestoreContext(context);
    }

    RestoreContext(context);
  }

  template <typename R>
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
      r = my::make_unique<R>();
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

protected:
  json_t * SaveContext(char const * name = nullptr)
  {
    json_t * context = m_json;
    if (name)
      m_json = my::GetJSONObligatoryField(context, name);
    return context;
  }

  void RestoreContext(json_t * context)
  {
    if (context)
      m_json = context;
  }

  my::Json m_jsonObject;
  json_t * m_json = nullptr;
};
}  // namespace coding
