#pragma once

#include "jansson_handle.hpp"

#include "base/exception.hpp"
#include "base/string_utils.hpp"

#include <memory>
#include <string>
#include <vector>

#include "3party/jansson/src/jansson.h"

namespace my
{
struct JSONDecRef
{
  void operator()(json_t * root) const { json_decref(root); }
};

using JSONPtr = std::unique_ptr<json_t, JSONDecRef>;

inline JSONPtr NewJSONObject() { return JSONPtr(json_object()); }
inline JSONPtr NewJSONArray() { return JSONPtr(json_array()); }
inline JSONPtr NewJSONString(std::string const & s) { return JSONPtr(json_string(s.c_str())); }

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

json_t * GetJSONObligatoryField(json_t * root, std::string const & field);
json_t * GetJSONOptionalField(json_t * root, std::string const & field);
}  // namespace my

inline void FromJSON(json_t * root, json_t *& value) { value = root; }

void FromJSONObject(json_t * root, std::string const & field, double & result);
void FromJSONObject(json_t * root, std::string const & field, json_int_t & result);

void FromJSONObjectOptionalField(json_t * root, std::string const & field, json_int_t & result);
void FromJSONObjectOptionalField(json_t * root, std::string const & field, double & result);
void FromJSONObjectOptionalField(json_t * root, std::string const & field, bool & result,
                                 bool def = false);
void FromJSONObjectOptionalField(json_t * root, std::string const & field, json_t *& result);

void ToJSONObject(json_t & root, std::string const & field, double value);
void ToJSONObject(json_t & root, std::string const & field, int value);

void FromJSON(json_t * root, std::string & result);
inline my::JSONPtr ToJSON(std::string const & s) { return my::NewJSONString(s); }

void FromJSONObject(json_t * root, std::string const & field, std::string & result);
void ToJSONObject(json_t & root, std::string const & field, std::string const & value);

template <typename T>
void FromJSONObject(json_t * root, std::string const & field, std::vector<T> & result)
{
  auto * arr = my::GetJSONObligatoryField(root, field);
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
}

template <typename T>
void ToJSONObject(json_t & root, std::string const & field, std::vector<T> const & values)
{
  auto arr = my::NewJSONArray();
  for (auto const & value : values)
    json_array_append_new(arr.get(), ToJSON(value).release());
  json_object_set_new(&root, field.c_str(), arr.release());
}

void FromJSONObjectOptionalField(json_t * root, std::string const & field, std::string & result);

template <typename T>
void FromJSONObjectOptionalField(json_t * root, std::string const & field, std::vector<T> & result)
{
  json_t * arr = my::GetJSONOptionalField(root, field);
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

namespace strings
{
void FromJSONObject(json_t * root, std::string const & field, UniString & result);
void ToJSONObject(json_t & root, std::string const & field, UniString const & value);
}
