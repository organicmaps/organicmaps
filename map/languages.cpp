#include "languages.hpp"

#include "../defines.hpp"

#include "../platform/platform.hpp"
#include "../platform/preferred_languages.hpp"
#include "../platform/settings.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/multilang_utf8_string.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/sstream.hpp"


#define DEFAULT_LANGUAGES "default"
#define LANGUAGES_FILE "languages.txt"
#define LANG_DELIMETER "|"
#define SETTING_LANG_KEY "languages_priority"


namespace languages
{
  static int8_t gDefaultPriorities[] =
  {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 62, 63
  };

  int8_t const * GetCurrentPriorities()
  {
    return gDefaultPriorities;
  }

  static void SetPreferableLanguages(vector<string> const & langCodes)
  {
    CHECK_EQUAL(langCodes.size(), MAX_SUPPORTED_LANGUAGES, ());
    for (size_t i = 0; i < langCodes.size(); ++i)
    {
      int8_t const index = StringUtf8Multilang::GetLangIndex(langCodes[i]);
      if (index >= 0)
        gDefaultPriorities[index] = i;
      else
      {
        ASSERT(false, ("Invalid language code"));
      }
      CHECK_GREATER_OR_EQUAL(gDefaultPriorities[i], 0, ("Unsupported language", langCodes[i]));
    }
  }

  /// sorts outLanguages according to langCodes order
  static void Sort(vector<string> const & codes, CodesAndNamesT & langs)
  {
    size_t ind = 0; // first unsorted position

    // make selection sort
    for (size_t code = 0; code < codes.size(); ++code)
    {
      for (size_t i = ind; i < langs.size(); ++i)
      {
        if (langs[i].first == codes[code])
        {
          if (ind != i) swap(langs[ind], langs[i]);
          ++ind;
          break;
        }
      }
    }
  }

  struct Collector
  {
    CodesT & m_out;
    Collector(CodesT & out) : m_out(out) {}
    void operator()(string const & str)
    {
      m_out.push_back(str);
    }
  };

  void GetCurrentSettings(CodesT & outLangCodes)
  {
    CodesAndNamesT res;
    GetCurrentSettings(res);

    outLangCodes.clear();
    for (CodesAndNamesT::iterator it = res.begin(); it != res.end(); ++it)
      outLangCodes.push_back(it->first);
  }

  void GetCurrentSettings(CodesAndNamesT & outLanguages)
  {
    string settingsString;
    // get preffered languages from the system
    if (!Settings::Get(SETTING_LANG_KEY, settingsString))
      settingsString = languages::PreferredLanguages();

    CodesT currentCodes;
    Collector c(currentCodes);
    strings::Tokenize(settingsString, LANG_DELIMETER, c);

    if (GetSupportedLanguages(outLanguages))
      Sort(currentCodes, outLanguages);
  }

  void SaveSettings(CodesT const & langs)
  {
    CHECK_EQUAL(langs.size(), MAX_SUPPORTED_LANGUAGES, ());
    string const saveString = strings::JoinStrings(langs.begin(), langs.end(), LANG_DELIMETER);
    Settings::Set(SETTING_LANG_KEY, saveString);

    // apply new settings
    SetPreferableLanguages(langs);
  }

  bool GetSupportedLanguages(CodesAndNamesT & outLanguages)
  {
    outLanguages.clear();

    string buffer;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(LANGUAGES_FILE)).ReadAsString(buffer);
    }
    catch (RootException const &)
    {
      return false;
    }

    istringstream stream(buffer);
    for (size_t i = 0; (i < MAX_SUPPORTED_LANGUAGES) && stream.good(); ++i)
    {
      string line;
      getline(stream, line);

      size_t const delimIndex = line.find(LANG_DELIMETER);
      if (delimIndex != string::npos)
        outLanguages.push_back(make_pair(line.substr(0, delimIndex), line.substr(delimIndex + 1)));
    }

    return !outLanguages.empty();
  }
}
