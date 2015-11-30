#pragma once

#include "jansson_handle.hpp"

#include "base/exception.hpp"

#include <jansson.h>


namespace my
{

class Json
{
  JsonHandle m_handle;

public:
  DECLARE_EXCEPTION(Exception, RootException);

  explicit Json(char const * s)
  {
    json_error_t jsonError;
    m_handle.AttachNew(json_loads(s, 0, &jsonError));
    if (!m_handle)
      MYTHROW(Exception, (jsonError.line, jsonError.text));
  }

  json_t * get() const { return m_handle.get(); }
};

}
