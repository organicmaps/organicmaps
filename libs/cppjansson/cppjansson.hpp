#pragma once

#include "jansson_handle.hpp"

#include "base/exception.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <jansson.h>

namespace base
{
struct JSONDecRef
{
  void operator()(json_t * root) const
  {
    if (root)
      json_decref(root);
  }
};

using JSONPtr = std::unique_ptr<json_t, JSONDecRef>;

inline JSONPtr NewJSONObject()
{
  return JSONPtr(json_object());
}
inline JSONPtr NewJSONArray()
{
  return JSONPtr(json_array());
}
inline JSONPtr NewJSONString(std::string const & s)
{
  return JSONPtr(json_string(s.c_str()));
}
inline JSONPtr NewJSONInt(json_int_t value)
{
  return JSONPtr(json_integer(value));
}
inline JSONPtr NewJSONReal(double value)
{
  return JSONPtr(json_real(value));
}
inline JSONPtr NewJSONBool(bool value)
{
  return JSONPtr(value ? json_true() : json_false());
}
inline JSONPtr NewJSONNull()
{
  return JSONPtr(json_null());
}

class Json
{
public:
  DECLARE_EXCEPTION(Exception, RootException);

  Json() = default;
  explicit Json(std::string const & s) { ParseFrom(s); }
  explicit Json(char const * s) { ParseFrom(s); }
  explicit Json(JSONPtr && json) { m_handle.AttachNew(json.release()); }

  Json GetDeepCopy() const
  {
    Json copy;
    copy.m_handle.AttachNew(get_deep_copy());
    return copy;
  }
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

JSONPtr LoadFromString(std::string const & str);
std::string DumpToString(JSONPtr const & json, size_t flags = 0);

json_t * GetJSONObligatoryField(json_t * root, std::string const & field);
json_t const * GetJSONObligatoryField(json_t const * root, std::string const & field);
json_t * GetJSONObligatoryField(json_t * root, char const * field);
json_t const * GetJSONObligatoryField(json_t const * root, char const * field);
json_t * GetJSONOptionalField(json_t * root, std::string const & field);
json_t const * GetJSONOptionalField(json_t const * root, std::string const & field);
json_t * GetJSONOptionalField(json_t * root, char const * field);
json_t const * GetJSONOptionalField(json_t const * root, char const * field);

template <class First>
inline json_t const * GetJSONObligatoryFieldByPath(json_t const * root, First && path)
{
  return GetJSONObligatoryField(root, std::forward<First>(path));
}

template <class First, class... Paths>
inline json_t const * GetJSONObligatoryFieldByPath(json_t const * root, First && path, Paths &&... paths)
{
  json_t const * newRoot = GetJSONObligatoryFieldByPath(root, std::forward<First>(path));
  return GetJSONObligatoryFieldByPath(newRoot, std::forward<Paths>(paths)...);
}

template <class First>
inline json_t * GetJSONObligatoryFieldByPath(json_t * root, First && path)
{
  return GetJSONObligatoryField(root, std::forward<First>(path));
}

template <class First, class... Paths>
inline json_t * GetJSONObligatoryFieldByPath(json_t * root, First && path, Paths &&... paths)
{
  json_t * newRoot = GetJSONObligatoryFieldByPath(root, std::forward<First>(path));
  return GetJSONObligatoryFieldByPath(newRoot, std::forward<Paths>(paths)...);
}

bool JSONIsNull(json_t const * root);
}  // namespace base

template <typename T>
T FromJSON(json_t const * root)
{
  T result{};
  FromJSON(root, result);
  return result;
}

inline void FromJSON(json_t * root, json_t *& value)
{
  value = root;
}
inline void FromJSON(json_t const * root, json_t const *& value)
{
  value = root;
}

void FromJSON(json_t const * root, double & result);
void FromJSON(json_t const * root, bool & result);

template <typename T, typename std::enable_if<std::is_integral<T>::value, void>::type * = nullptr>
void FromJSON(json_t const * root, T & result)
{
  if (!json_is_number(root))
    MYTHROW(base::Json::Exception, ("Object must contain a json number."));
  result = static_cast<T>(json_integer_value(root));
}

std::string FromJSONToString(json_t const * root);

template <typename T>
T FromJSONObject(json_t const * root, char const * field)
{
  auto const * json = base::GetJSONObligatoryField(root, field);
  try
  {
    return FromJSON<T>(json);
  }
  catch (base::Json::Exception const & e)
  {
    MYTHROW(base::Json::Exception, ("An error occured while parsing field", field, e.Msg()));
  }
}

template <typename T>
void FromJSONObject(json_t * root, std::string const & field, T & result)
{
  auto * json = base::GetJSONObligatoryField(root, field);
  try
  {
    FromJSON(json, result);
  }
  catch (base::Json::Exception const & e)
  {
    MYTHROW(base::Json::Exception, ("An error occured while parsing field", field, e.Msg()));
  }
}

template <typename T>
std::optional<T> FromJSONObjectOptional(json_t const * root, char const * field)
{
  auto * json = base::GetJSONOptionalField(root, field);
  if (!json)
    return {};

  std::optional<T> result{T{}};
  FromJSON(json, *result);
  return result;
}

template <typename T>
void FromJSONObjectOptionalField(json_t const * root, std::string const & field, T & result)
{
  auto * json = base::GetJSONOptionalField(root, field);
  if (!json)
  {
    result = T{};
    return;
  }
  FromJSON(json, result);
}

template <typename T, typename std::enable_if<std::is_integral<T>::value, void>::type * = nullptr>
inline base::JSONPtr ToJSON(T value)
{
  return base::NewJSONInt(value);
}
inline base::JSONPtr ToJSON(double value)
{
  return base::NewJSONReal(value);
}
inline base::JSONPtr ToJSON(bool value)
{
  return base::NewJSONBool(value);
}
inline base::JSONPtr ToJSON(char const * s)
{
  return base::NewJSONString(s);
}

template <typename T>
void ToJSONArray(json_t & root, T const & value)
{
  json_array_append_new(&root, ToJSON(value).release());
}

inline void ToJSONArray(json_t & parent, base::JSONPtr & child)
{
  json_array_append_new(&parent, child.release());
}

inline void ToJSONArray(json_t & parent, json_t & child)
{
  json_array_append_new(&parent, &child);
}

template <typename T>
void ToJSONObject(json_t & root, char const * field, T const & value)
{
  json_object_set_new(&root, field, ToJSON(value).release());
}

inline void ToJSONObject(json_t & parent, char const * field, base::JSONPtr && child)
{
  json_object_set_new(&parent, field, child.release());
}

inline void ToJSONObject(json_t & parent, char const * field, base::JSONPtr & child)
{
  json_object_set_new(&parent, field, child.release());
}

inline void ToJSONObject(json_t & parent, char const * field, json_t & child)
{
  json_object_set_new(&parent, field, &child);
}

template <typename T>
void ToJSONObject(json_t & root, std::string const & field, T const & value)
{
  ToJSONObject(root, field.c_str(), value);
}

inline void ToJSONObject(json_t & parent, std::string const & field, base::JSONPtr && child)
{
  ToJSONObject(parent, field.c_str(), std::move(child));
}

inline void ToJSONObject(json_t & parent, std::string const & field, base::JSONPtr & child)
{
  ToJSONObject(parent, field.c_str(), child);
}

inline void ToJSONObject(json_t & parent, std::string const & field, json_t & child)
{
  ToJSONObject(parent, field.c_str(), child);
}

template <typename T>
void FromJSONObject(json_t * root, std::string const & field, std::vector<T> & result)
{
  auto * arr = base::GetJSONObligatoryField(root, field);
  if (!json_is_array(arr))
    MYTHROW(base::Json::Exception, ("The field", field, "must contain a json array."));
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
  auto * arr = base::GetJSONOptionalField(root, field);
  if (!arr || base::JSONIsNull(arr))
  {
    result.clear();
    return false;
  }
  if (!json_is_array(arr))
    MYTHROW(base::Json::Exception, ("The field", field, "must contain a json array."));
  size_t const sz = json_array_size(arr);
  result.resize(sz);
  for (size_t i = 0; i < sz; ++i)
    FromJSON(json_array_get(arr, i), result[i]);
  return true;
}

template <typename T>
void ToJSONObject(json_t & root, char const * field, std::vector<T> const & values)
{
  auto arr = base::NewJSONArray();
  for (auto const & value : values)
    json_array_append_new(arr.get(), ToJSON(value).release());
  json_object_set_new(&root, field, arr.release());
}

template <typename T>
void ToJSONObject(json_t & root, std::string const & field, std::vector<T> const & values)
{
  ToJSONObject(root, field.c_str(), values);
}

template <typename T>
void FromJSONObjectOptionalField(json_t * root, std::string const & field, std::vector<T> & result)
{
  FromJSONObjectOptionalField(const_cast<json_t const *>(root), field, result);
}

template <typename T>
void FromJSONObjectOptionalField(json_t const * root, std::string const & field, std::vector<T> & result)
{
  json_t const * arr = base::GetJSONOptionalField(root, field);
  if (!arr)
  {
    result.clear();
    return;
  }
  if (!json_is_array(arr))
    MYTHROW(base::Json::Exception, ("The field", field, "must contain a json array."));
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
void FromJSON(json_t const * root, std::string & result);
inline base::JSONPtr ToJSON(std::string const & s)
{
  return base::NewJSONString(s);
}
}  // namespace std

namespace strings
{
void FromJSON(json_t const * root, UniString & result);
base::JSONPtr ToJSON(UniString const & s);
}  // namespace strings
