#include "cppjansson.hpp"

#include <type_traits>

namespace
{
template <typename T>
std::string FromJSONToString(json_t const * root)
{
  T result;
  FromJSON(root, result);
  // TODO(AB): Is std::to_string faster?
  return strings::to_string(result);
}
}  // namespace

namespace base
{
json_t * GetJSONObligatoryField(json_t * root, std::string const & field)
{
  return GetJSONObligatoryField(root, field.c_str());
}

json_t * GetJSONObligatoryField(json_t * root, char const * field)
{
  return const_cast<json_t *>(GetJSONObligatoryField(const_cast<json_t const *>(root), field));
}

json_t const * GetJSONObligatoryField(json_t const * root, std::string const & field)
{
  return GetJSONObligatoryField(root, field.c_str());
}

json_t const * GetJSONObligatoryField(json_t const * root, char const * field)
{
  auto * value = base::GetJSONOptionalField(root, field);
  if (!value)
    MYTHROW(base::Json::Exception, ("Obligatory field", field, "is absent."));
  return value;
}

json_t * GetJSONOptionalField(json_t * root, std::string const & field)
{
  return GetJSONOptionalField(root, field.c_str());
}

json_t * GetJSONOptionalField(json_t * root, char const * field)
{
  return const_cast<json_t *>(GetJSONOptionalField(const_cast<json_t const *>(root), field));
}

json_t const * GetJSONOptionalField(json_t const * root, std::string const & field)
{
  return GetJSONOptionalField(root, field.c_str());
}

json_t const * GetJSONOptionalField(json_t const * root, char const * field)
{
  if (!json_is_object(root))
    MYTHROW(base::Json::Exception, ("Bad json object while parsing", field));
  return json_object_get(root, field);
}

bool JSONIsNull(json_t const * root)
{
  return json_is_null(root);
}

std::string DumpToString(JSONPtr const & json, size_t flags)
{
  std::string result;
  size_t size = json_dumpb(json.get(), nullptr, 0, flags);
  if (size == 0)
    MYTHROW(base::Json::Exception, ("Zero size JSON while serializing"));

  result.resize(size);
  if (size != json_dumpb(json.get(), &result.front(), size, flags))
    MYTHROW(base::Json::Exception, ("Wrong size JSON written while serializing"));

  return result;
}

JSONPtr LoadFromString(std::string const & str)
{
  json_error_t jsonError = {};
  json_t * result = json_loads(str.c_str(), 0, &jsonError);
  if (!result)
    MYTHROW(base::Json::Exception, (jsonError.text));
  return JSONPtr(result);
}

}  // namespace base

void FromJSON(json_t const * root, double & result)
{
  if (!json_is_number(root))
    MYTHROW(base::Json::Exception, ("Object must contain a json number."));
  result = json_number_value(root);
}

void FromJSON(json_t const * root, bool & result)
{
  if (!json_is_true(root) && !json_is_false(root))
    MYTHROW(base::Json::Exception, ("Object must contain a boolean value."));
  result = json_is_true(root);
}

std::string FromJSONToString(json_t const * root)
{
  if (json_is_string(root))
    return FromJSONToString<std::string>(root);

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
void FromJSON(json_t const * root, std::string & result)
{
  if (!json_is_string(root))
    MYTHROW(base::Json::Exception, ("The field must contain a json string."));
  result = json_string_value(root);
}
}  // namespace std

namespace strings
{
void FromJSON(json_t const * root, UniString & result)
{
  std::string s;
  FromJSON(root, s);
  result = MakeUniString(s);
}

base::JSONPtr ToJSON(UniString const & s)
{
  return ToJSON(ToUtf8(s));
}
}  // namespace strings
