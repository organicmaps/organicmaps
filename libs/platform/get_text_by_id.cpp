#include "platform/get_text_by_id.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "cppjansson/cppjansson.hpp"

#include <algorithm>

namespace platform
{
using std::string;

namespace
{
string const kDefaultLanguage = "en";

string GetTextSourceString(platform::TextSource textSource)
{
  switch (textSource)
  {
  case platform::TextSource::TtsSound: return string("sound-strings");
  case platform::TextSource::Countries: return string("countries-strings");
  }
  ASSERT(false, ());
  return string();
}
}  // namespace

bool GetJsonBuffer(platform::TextSource textSource, string const & localeName, string & jsonBuffer)
{
  string const pathToJson = base::JoinPath(GetTextSourceString(textSource), localeName + ".json", "localize.json");

  try
  {
    jsonBuffer.clear();
    GetPlatform().GetReader(pathToJson)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Can't open", localeName, "localization file:", pathToJson, ex.what()));
    return false;  // No json file for localeName
  }
  return true;
}

TGetTextByIdPtr GetTextById::Create(string const & jsonBuffer, string const & localeName)
{
  TGetTextByIdPtr result(new GetTextById(jsonBuffer, localeName));
  if (!result->IsValid())
  {
    ASSERT(false, ("Can't create a GetTextById instance from a json file. localeName=", localeName));
    return nullptr;
  }
  return result;
}

TGetTextByIdPtr GetTextByIdFactory(TextSource textSource, string const & localeName)
{
  string jsonBuffer;
  if (GetJsonBuffer(textSource, localeName, jsonBuffer))
    return GetTextById::Create(jsonBuffer, localeName);

  if (GetJsonBuffer(textSource, kDefaultLanguage, jsonBuffer))
    return GetTextById::Create(jsonBuffer, kDefaultLanguage);

  ASSERT(false, ("Can't find translate for default language. (Lang:", localeName, ")"));
  return nullptr;
}

TGetTextByIdPtr ForTestingGetTextByIdFactory(string const & jsonBuffer, string const & localeName)
{
  return GetTextById::Create(jsonBuffer, localeName);
}

GetTextById::GetTextById(string const & jsonBuffer, string const & localeName) : m_locale(localeName)
{
  if (jsonBuffer.empty())
  {
    ASSERT(false, ("No json files found."));
    return;
  }

  base::Json root(jsonBuffer.c_str());
  if (root.get() == nullptr)
  {
    ASSERT(false, ("Cannot parse the json file."));
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

string GetTextById::operator()(string const & textId) const
{
  auto const textIt = m_localeTexts.find(textId);
  if (textIt == m_localeTexts.end())
    return string();
  return textIt->second;
}

TTranslations GetTextById::GetAllSortedTranslations() const
{
  TTranslations all;
  all.reserve(m_localeTexts.size());
  for (auto const & tr : m_localeTexts)
    all.emplace_back(tr.first, tr.second);
  using TValue = TTranslations::value_type;
  sort(all.begin(), all.end(), [](TValue const & v1, TValue const & v2) { return v1.second < v2.second; });
  return all;
}
}  // namespace platform
