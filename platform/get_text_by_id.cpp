#include "platform/get_text_by_id.hpp"
#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include "3party/jansson/myjansson.hpp"

#include "std/target_os.hpp"

namespace
{
string GetTextSourceString(platform::TextSource textSouce)
{
#if defined(OMIM_OS_ANDROID) || defined(OMIM_OS_IPHONE)
  switch (textSouce)
  {
    case platform::TextSource::TtsSound:
      return "sound-strings";
  }
#else
  switch (textSouce)
  {
    case platform::TextSource::TtsSound:
      return "../data/sound-strings";
  }
#endif
  ASSERT(false, ());
}
}  // namespace

namespace platform
{
GetTextById::GetTextById(TextSource textSouce, string const & localeName)
{
  string const pathToJson = my::JoinFoldersToPath(
      {GetTextSourceString(textSouce), localeName + ".json"}, "localize.json");

  // @TODO(vbykoianko) Add assert if locale path pathToJson is not valid.
  string jsonBuffer;
  ReaderPtr<Reader>(GetPlatform().GetReader(pathToJson)).ReadAsString(jsonBuffer);

  if (jsonBuffer.empty())
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

  char const * key = nullptr;
  json_t * value = nullptr;
  json_object_foreach(root.get(), key, value)
  {
    ASSERT(key, ());
    ASSERT(value, ());
    char const * const valueStr = json_string_value(value);
    ASSERT(valueStr, ());
    m_localeTexts[key] = valueStr;
  }
  ASSERT_EQUAL(m_localeTexts.size(), json_object_size(root.get()), ());
}

pair<string, bool> GetTextById::operator()(string const & textId) const
{
  if (!IsValid())
    return make_pair(textId, false);

  auto const textIt = m_localeTexts.find(textId);
  if (textIt == m_localeTexts.end())
    return make_pair(textId, false);
  return make_pair(textIt->second, true);
}
}  // namespace platform
