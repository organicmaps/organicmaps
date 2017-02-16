#pragma once

#include "jansson_handle.hpp"

#include "base/exception.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/src/jansson.h"

#include "std/string.hpp"
#include "std/vector.hpp"

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

void FromJSON(json_t * root, string & result);
inline void FromJSON(json_t * root, json_t *& value) { value = root; }
void FromJSONObject(json_t * root, string const & field, string & result);
void FromJSONObject(json_t * root, string const & field, strings::UniString & result);
void FromJSONObject(json_t * root, string const & field, double & result);
void FromJSONObject(json_t * root, string const & field, json_int_t & result);

template <typename T>
void FromJSONObject(json_t * root, string const & field, vector<T> & result)
{
  json_t * arr = json_object_get(root, field.c_str());
  if (!arr)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, string & result);
void FromJSONObjectOptionalField(json_t * root, string const & field, json_int_t & result);
void FromJSONObjectOptionalField(json_t * root, string const & field, double & result);
void FromJSONObjectOptionalField(json_t * root, string const & field, bool & result, bool def = false);
void FromJSONObjectOptionalField(json_t * root, string const & field, json_t *& result);

template <typename T>
void FromJSONObjectOptionalField(json_t * root, string const & field, vector<T> & result)
{
  json_t * arr = json_object_get(root, field.c_str());
  if (!arr)
  {
    result.clear();
    return;
  }
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
}
}  // namespace my
