#include "3party/jansson/myjansson.hpp"

#include <type_traits>

using namespace std;

namespace
{
template <typename T>
void ReadIntegral(json_t * root, string const & field, T & result)
{
  static_assert(is_integral<T>::value, "T must be an integral type");

  auto * val = my::GetJSONObligatoryField(root, field);
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = static_cast<T>(json_integer_value(val));
}

template <typename T>
void ReadIntegralOptional(json_t * root, string const & field, T & result)
{
  static_assert(is_integral<T>::value, "T must be an integral type");

  auto * val = my::GetJSONOptionalField(root, field);
  if (!val)
  {
    result = 0;
    return;
  }
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = static_cast<T>(json_integer_value(val));
}
}  // namespace

namespace my
{
json_t * GetJSONObligatoryField(json_t * root, std::string const & field)
{
  auto * value = my::GetJSONOptionalField(root, field);
  if (!value)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  return value;
}

json_t * GetJSONOptionalField(json_t * root, std::string const & field)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  return json_object_get(root, field.c_str());
}

bool JSONIsNull(json_t * root) { return json_is_null(root); }
}  // namespace my

void FromJSONObject(json_t * root, string const & field, double & result)
{
  auto * val = my::GetJSONObligatoryField(root, field);
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_number_value(val);
}

void FromJSONObject(json_t * root, string const & field, int & result)
{
  ReadIntegral(root, field, result);
}

void FromJSONObject(json_t * root, string const & field, int64_t & result)
{
  ReadIntegral(root, field, result);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, int & result)
{
  ReadIntegralOptional(root, field, result);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, int64_t & result)
{
  ReadIntegralOptional(root, field, result);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, double & result)
{
  json_t * val = my::GetJSONOptionalField(root, field);
  if (!val)
  {
    result = 0.0;
    return;
  }
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_number_value(val);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, bool & result, bool def)
{
  json_t * val = my::GetJSONOptionalField(root, field);
  if (!val)
  {
    result = def;
    return;
  }
  if (!json_is_boolean(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a boolean value."));
  result = json_is_true(val);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, json_t *& result)
{
  json_t * obj = my::GetJSONOptionalField(root, field);
  if (!obj)
  {
    result = nullptr;
    return;
  }
  if (!json_is_object(obj))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json object."));
  FromJSON(obj, result);
}

void ToJSONObject(json_t & root, string const & field, double value)
{
  json_object_set_new(&root, field.c_str(), json_real(value));
}

void ToJSONObject(json_t & root, string const & field, int value)
{
  ToJSONObject(root, field, static_cast<int64_t>(value));
}

void ToJSONObject(json_t & root, std::string const & field, int64_t value)
{
  json_object_set_new(&root, field.c_str(), json_integer(value));
}

void FromJSON(json_t * root, string & result)
{
  if (!json_is_string(root))
    MYTHROW(my::Json::Exception, ("The field must contain a json string."));
  result = json_string_value(root);
}

void FromJSONObject(json_t * root, string const & field, string & result)
{
  auto * val = my::GetJSONObligatoryField(root, field);
  if (!json_is_string(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json string."));
  result = json_string_value(val);
}

void ToJSONObject(json_t & root, string const & field, string const & value)
{
  json_object_set_new(&root, field.c_str(), json_string(value.c_str()));
}

void FromJSONObjectOptionalField(json_t * root, string const & field, string & result)
{
  auto * val = my::GetJSONOptionalField(root, field);
  if (!val || my::JSONIsNull(val))
  {
    result.clear();
    return;
  }
  if (!json_is_string(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json string."));
  result = json_string_value(val);
}

namespace strings
{
void FromJSONObject(json_t * root, string const & field, UniString & result)
{
  string s;
  FromJSONObject(root, field, s);
  result = strings::MakeUniString(s);
}

void ToJSONObject(json_t & root, string const & field, UniString const & value)
{
  return ToJSONObject(root, field, strings::ToUtf8(value));
}
}  // namespace strings
