#include "platform/get_text_by_id.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "3party/jansson/myjansson.hpp"


namespace platform
{
GetTextById::GetTextById(string const & textsDir, string const & localeName)
{
  string const pathToJson = textsDir + "/" + localeName + ".json/localize.json";

  string jsonBuffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(pathToJson)).ReadAsString(jsonBuffer);

  if (jsonBuffer == "")
  {
    ASSERT(false, ("No json files found at the path", pathToJson));
    return;
  }

  my::Json root(jsonBuffer.c_str());
  if (root.get() == nullptr)
  {
    ASSERT(false, ("Cannot parse the json file found at the path", pathToJson));
    return;
  }

  const char * key = nullptr;
  json_t * value = nullptr;
  json_object_foreach(root.get(), key, value)
  {
    ASSERT(key, ());
    ASSERT(value, ());
    const char * valueStr = json_string_value(value);
    ASSERT(valueStr, ());
    m_localeTexts[key] = valueStr;
  }
  ASSERT_EQUAL(m_localeTexts.size(), json_object_size(root.get()), ());
}

string GetTextById::operator()(string const & textId) const
{
  if (!IsValid())
    return textId;

  auto const textIt = m_localeTexts.find(textId);
  if (textIt == m_localeTexts.end())
    return textId;
  return textIt->second;
}
}  // namespace platform
