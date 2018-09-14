#include "3party/jansson/myjansson.hpp"

#include <type_traits>

using namespace std;

namespace
{
template <typename T>
string FromJSONToString(json_t * root)
{
  T result;
  FromJSON(root, result);
  return strings::to_string(result);
}
}  // namespace

namespace base
{
json_t * GetJSONObligatoryField(json_t * root, std::string const & field)
{
  auto * value = base::GetJSONOptionalField(root, field);
  if (!value)
    MYTHROW(base::Json::Exception, ("Obligatory field", field, "is absent."));
  return value;
}

json_t * GetJSONOptionalField(json_t * root, std::string const & field)
{
  if (!json_is_object(root))
    MYTHROW(base::Json::Exception, ("Bad json object while parsing", field));
  return json_object_get(root, field.c_str());
}

bool JSONIsNull(json_t * root) { return json_is_null(root); }
}  // namespace base

void FromJSON(json_t * root, double & result)
{
  if (!json_is_number(root))
    MYTHROW(base::Json::Exception, ("Object must contain a json number."));
  result = json_number_value(root);
}

void FromJSON(json_t * root, bool & result)
{
  if (!json_is_true(root) && !json_is_false(root) )
    MYTHROW(base::Json::Exception, ("Object must contain a boolean value."));
  result = json_is_true(root);
}

string FromJSONToString(json_t * root)
{
  if (json_is_string(root))
    return FromJSONToString<string>(root);

  if (json_is_integer(root))
    return FromJSONToString<json_int_t>(root);

  if (json_is_real(root))
    return FromJSONToString<double>(root);

  if (json_is_boolean(root))
    return FromJSONToString<bool>(root);

  MYTHROW(base::Json::Exception, ("Unexpected json type"));
}

namespace std
{
void FromJSON(json_t * root, string & result)
{
  if (!json_is_string(root))
    MYTHROW(base::Json::Exception, ("The field must contain a json string."));
  result = json_string_value(root);
}
}  // namespace std

namespace strings
{
void FromJSON(json_t * root, UniString & result)
{
  string s;
  FromJSON(root, s);
  result = MakeUniString(s);
}

base::JSONPtr ToJSON(UniString const & s) { return ToJSON(ToUtf8(s)); }
}  // namespace strings
