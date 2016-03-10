#include "3party/jansson/myjansson.hpp"

namespace my
{
void FromJSON(json_t * root, string & result)
{
  result = string(json_string_value(root));
}

void FromJSONObject(json_t * root, string const & field, string & result)
{
  if (!json_is_object(root))
    MYTHROW(my::Json::Exception, ("Bad json object when parsing", field));
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
    MYTHROW(my::Json::Exception, ("Bad json object when parsing", field));
  json_t * val = json_object_get(root, field.c_str());
  if (!val)
    MYTHROW(my::Json::Exception, ("Obligatory field", field, "is absent."));
  if (!json_is_number(val))
    MYTHROW(my::Json::Exception, ("The field", field, "must contain a json number."));
  result = json_number_value(val);
}
}  // namespace my
