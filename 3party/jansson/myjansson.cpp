#include "3party/jansson/myjansson.hpp"

namespace my
{
void FromJSON(json_t * root, string & result)
{
  if (!json_is_string(root))
    MYTHROW(my::Json::Exception, ("The field must contain a json string."));
  result = string(json_string_value(root));
}

void FromJSONObject(json_t * root, string const & field, string & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_string(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json string."));
  result = string(json_string_value(val));
}

void FromJSONObject(json_t * root, string const & field, strings::UniString & result)
{
  string s;
  FromJSONObject(root, field, s);
  result = strings::MakeUniString(s);
}

void FromJSONObject(json_t * root, string const & field, double & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_number_value(val);
}

void FromJSONObject(json_t * root, string const & field, json_int_t & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_integer_value(val);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, string & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
  {
    result.clear();
    return;
  }
  if (!json_is_string(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json string."));
  result = string(json_string_value(val));
}

void FromJSONObjectOptionalField(json_t * root, string const & field, json_int_t & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
  {
    result = 0;
    return;
  }
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_integer_value(val);
}

void FromJSONObjectOptionalField(json_t * root, string const & field, double & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
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
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object while parsing", field));
  json_t * val = json_object_get(root, field.c_str());
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
  json_t * obj = json_object_get(root, field.c_str());
  if (!obj)
  {
    result = nullptr;
    return;
  }
  if (!json_is_object(obj))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json object."));
  FromJSON(obj, result);
}
}  // namespace my
