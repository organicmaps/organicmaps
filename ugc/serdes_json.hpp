#pragma once

#include "ugc/types.hpp"

#include "base/exception.hpp"

#include "3party/jansson/myjansson.hpp"

#include <cstdint>
#include <cstdlib>

namespace ugc
{
template <typename Sink>
class SerializerJson
{
public:
  SerializerJson(Sink & sink) : m_sink(sink) {}
  ~SerializerJson()
  {
    char * str = json_dumps(m_json.get(), 0);
    std::string json(str);
    std::free(static_cast<void *>(str));
    m_sink.Write(json.data(), json.size());
  }
  void operator()(bool const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint8_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint32_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(uint64_t const d, char const * name = nullptr) { ToJSONObject(*m_json, name, d); }
  void operator()(std::string const & s, char const * name = nullptr)
  {
    ToJSONObject(*m_json, name, s);
  }

  void operator()(TranslationKey const & key, char const * name = nullptr)
  {
    (*this)(key.m_key, name);
  }

  void operator()(Time const & t, char const * name = nullptr)
  {
    (*this)(ToDaysSinceEpoch(t), name);
  }

  void operator()(Sentiment sentiment, char const * name = nullptr)
  {
    switch (sentiment)
    {
    case Sentiment::Negative: return (*this)(false, name);
    case Sentiment::Positive: return (*this)(true, name);
    }
  }

  template <typename T>
  void operator()(vector<T> const & vs, char const * name = nullptr)
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

  void VisitRating(float const f, char const * name = nullptr)
  {
    CHECK_GREATER_OR_EQUAL(f, 0.0, ());
    auto const d = static_cast<double>(f);
    ToJSONObject(*m_json, name, d);
  }

  template <typename T>
  void VisitVarUint(T const & t, char const * name = nullptr)
  {
    ToJSONObject(*m_json, name, t);
  }

  void VisitLang(uint8_t const index, char const * name = nullptr)
  {
    ToJSONObject(*m_json, name, StringUtf8Multilang::GetLangByCode(index));
  }

private:
  template <typename Fn>
  void NewScopeWith(my::JSONPtr json_object, char const * name, Fn && fn)
  {
    my::JSONPtr safe_json = std::move(m_json);
    m_json = std::move(json_object);

    fn();

    if (safe_json && json_is_array(safe_json))
      json_array_append_new(safe_json.get(), m_json.release());
    else if (safe_json && json_is_object(safe_json))
      json_object_set_new(safe_json.get(), name, m_json.release());

    if (safe_json)
      m_json = std::move(safe_json);
  }

  my::JSONPtr m_json = nullptr;
  Sink & m_sink;
};

class DeserializerJsonV0
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  template <typename Source,
            typename std::enable_if<
              !std::is_convertible<Source, std::string>::value, Source>::type * = nullptr>
  explicit DeserializerJsonV0(Source & source)
  {
    std::string src(source.Size(), '\0');
    source.Read(static_cast<void *>(&src[0]), source.Size());
    m_jsonObject.ParseFrom(src);
    m_json = m_jsonObject.get();
  }

  explicit DeserializerJsonV0(std::string const & source)
    : m_jsonObject(source)
    , m_json(m_jsonObject.get())
  {
  }

  explicit DeserializerJsonV0(json_t * json)
    : m_json(json)
  {
  }

  void operator()(bool & d, char const * name = nullptr) { FromJSONObject(m_json, name, d); }
  void operator()(uint8_t & d, char const * name = nullptr) { FromJSONObject(m_json, name, d); }
  void operator()(uint32_t & d, char const * name = nullptr) { FromJSONObject(m_json, name, d); }
  void operator()(uint64_t & d, char const * name = nullptr) { FromJSONObject(m_json, name, d); }
  void operator()(std::string & s, char const * name = nullptr) { FromJSONObject(m_json, name, s); }
  void operator()(TranslationKey & key, char const * name = nullptr)
  {
    (*this)(key.m_key, name);
  }

  void operator()(Time & t, char const * name = nullptr)
  {
    uint32_t d = 0;
    FromJSONObject(m_json, name, d);
    t = FromDaysSinceEpoch(d);
  }

  void operator()(Sentiment & sentiment, char const * name = nullptr)
  {
    bool s = false;
    FromJSONObject(m_json, name, s);
    sentiment = s ? Sentiment::Positive : Sentiment::Negative;
  }

  template <typename T>
  void operator()(vector<T> & vs, char const * name = nullptr)
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

  void VisitRating(float & f, char const * name = nullptr)
  {
    double d = 0.0;
    FromJSONObject(m_json, name, d);
    f = static_cast<float>(d);
  }

  template <typename T>
  void VisitVarUint(T & t, char const * name = nullptr)
  {
    FromJSONObject(m_json, name, t);
  }

  void VisitLang(uint8_t & index, char const * name = nullptr)
  {
    std::string lang;
    FromJSONObject(m_json, name, lang);
    index = StringUtf8Multilang::GetLangIndex(lang);
  }

private:
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
}  // namespace ugc
