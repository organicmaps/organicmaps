#pragma once

#include "jansson_handle.hpp"

#include "base/exception.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
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
inline JSONPtr NewJSONInt(json_int_t value) { return JSONPtr(json_integer(value)); }
inline JSONPtr NewJSONReal(double value) { return JSONPtr(json_real(value)); }
inline JSONPtr NewJSONBool(bool value) { return JSONPtr(value ? json_true() : json_false()); }
inline JSONPtr NewJSONNull() { return JSONPtr(json_null()); }

class Json
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  Json() = default;
  explicit Json(std::string const & s) { ParseFrom(s); }
  explicit Json(char const * s) { ParseFrom(s); }

  void ParseFrom(std::string const & s) { ParseFrom(s.c_str()); }
  void ParseFrom(char const * s)
  {
    json_error_t jsonError;
    m_handle.AttachNew(json_loads(s, 0, &jsonError));
    if (!m_handle)
      MYTHROW(Exception, (jsonError.line, jsonError.text));
  }

  json_t * get() const { return m_handle.get(); }
  json_t * get_deep_copy() const { return json_deep_copy(get()); }

private:
  JsonHandle m_handle;
};

json_t * GetJSONObligatoryField(json_t * root, std::string const & field);
json_t * GetJSONOptionalField(json_t * root, std::string const & field);
bool JSONIsNull(json_t * root);
}  // namespace my

inline void FromJSON(json_t * root, json_t *& value) { value = root; }

void FromJSON(json_t * root, double & result);
void FromJSON(json_t * root, bool & result);

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, void>::type* = nullptr>
void FromJSON(json_t * root, T & result)
{
  if (!json_is_number(root))
    MYTHROW(my::Json::Exception, ("Object must contain a json number."));
  result = static_cast<T>(json_integer_value(root));
}

std::string FromJSONToString(json_t * root);

template <typename T>
void FromJSONObject(json_t * root, std::string const & field, T & result)
{
  auto * json = my::GetJSONObligatoryField(root, field);
  try
  {
    FromJSON(json, result);
  }
  catch (my::Json::Exception const & e)
  {
    MYTHROW(my::Json::Exception, ("An error occured while parsing field", field, e.Msg()));
  }
}

template <typename T>
void FromJSONObjectOptionalField(json_t * root, std::string const & field, T & result)
{
  auto * json = my::GetJSONOptionalField(root, field);
  if (!json)
  {
    result = T{};
    return;
  }
  FromJSON(json, result);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, void>::type* = nullptr>
inline my::JSONPtr ToJSON(T value) { return my::NewJSONInt(value); }
inline my::JSONPtr ToJSON(double value) { return my::NewJSONReal(value); }
inline my::JSONPtr ToJSON(bool value) { return my::NewJSONBool(value); }
inline my::JSONPtr ToJSON(char const * s) { return my::NewJSONString(s); }

template <typename T>
void ToJSONArray(json_t & root, T const & value)
{
  json_array_append_new(&root, ToJSON(value).release());
}

inline void ToJSONArray(json_t & parent, my::JSONPtr & child)
{
  json_array_append_new(&parent, child.release());
}

inline void ToJSONArray(json_t & parent, json_t & child)
{
  json_array_append_new(&parent, &child);
}

template <typename T>
void ToJSONObject(json_t & root, std::string const & field, T const & value)
{
  json_object_set_new(&root, field.c_str(), ToJSON(value).release());
}

inline void ToJSONObject(json_t & parent, std::string const & field, my::JSONPtr & child)
{
  json_object_set_new(&parent, field.c_str(), child.release());
}

inline void ToJSONObject(json_t & parent, std::string const & field, json_t & child)
{
  json_object_set_new(&parent, field.c_str(), &child);
}

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

// The function tries to parse array of values from a value
// corresponding to |field| in a json object corresponding to |root|.
// Returns true when the value is non-null and array is successfully
// parsed.  Returns false when there is no such |field| in the |root|
// or the value is null.  Also, the method may throw an exception in
// case of json parsing errors.
template <typename T>
bool FromJSONObjectOptional(json_t * root, std::string const & field, std::vector<T> & result)
{
  auto * arr = my::GetJSONOptionalField(root, field);
  if (!arr || my::JSONIsNull(arr))
  {
    result.clear();
    return false;
  }
  if (!json_is_array(arr))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json array."));
  size_t const sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
  return true;
}

template <typename T>
void ToJSONObject(json_t & root, std::string const & field, std::vector<T> const & values)
{
  auto arr = my::NewJSONArray();
  for (auto const & value : values)
    json_array_append_new(arr.get(), ToJSON(value).release());
  json_object_set_new(&root, field.c_str(), arr.release());
}

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

struct JSONFreeDeleter
{
  void operator()(char * buffer) const { free(buffer); }
};

namespace std
{
void FromJSON(json_t * root, std::string & result);
inline my::JSONPtr ToJSON(std::string const & s) { return my::NewJSONString(s); }
}  // namespace std

namespace strings
{
void FromJSON(json_t * root, UniString & result);
my::JSONPtr ToJSON(UniString const & s);
}  // namespace strings
