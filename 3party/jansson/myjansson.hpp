#include <jansson.h>

#include "../../base/exception.hpp"
#include "../../std/algorithm.hpp"

namespace my
{

class Json
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  explicit Json(json_t * pJson) : m_pJson(pJson)
  {
  }

  explicit Json(char const * s)
  {
    json_error_t jsonError;
    m_pJson = json_loads(s, 0, &jsonError);
    if (!m_pJson)
      MYTHROW(Exception, (jsonError.line, jsonError.text));
  }

  ~Json()
  {
    if (m_pJson)
      json_decref(m_pJson);
  }

  Json(Json const & json) : m_pJson(json.m_pJson)
  {
    if (m_pJson)
      json_incref(m_pJson);
  }

  Json & operator = (Json const & json)
  {
    Json tmp(json);
    tmp.swap(*this);
    return *this;
  }

  void swap(Json & json)
  {
    ::swap(m_pJson, json.m_pJson);
  }

  json_t * get() const
  {
    return m_pJson;
  }

  json_t * operator -> () const
  {
    return m_pJson;
  }

  operator json_t * () const
  {
    return m_pJson;
  }

private:
  json_t * m_pJson;
};

}

