#include "languages.hpp"
#include "settings.hpp"

#include "../base/logging.hpp"
#include "../base/string_utils.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/strutil.hpp"

#include "../platform/platform.hpp"

#include "../std/algorithm.hpp"
#include "../std/sstream.hpp"

#define DEFAULT_LANGUAGES "default"
#define MAX_SUPPORTED_LANGUAGES 64
#define LANGUAGES_FILE "languages.txt"
#define LANG_DELIMETER "|"
#define SETTING_LANG_KEY "languages_priority"

namespace languages
{
  /// sorts outLanguages according to langCodes order
  static void Sort(vector<string> const & langCodes, CodesAndNamesT & outLanguages)
  {
    CodesAndNamesT result;
    for (size_t i = 0; i < langCodes.size(); ++i)
    {
      for (CodesAndNamesT::iterator it = outLanguages.begin(); it != outLanguages.end(); ++it)
      {
        if (langCodes[i] == it->first)
        {
          result.push_back(*it);
          outLanguages.erase(it);
          break;
        }
      }
    }
    // copy all languages left
    for (CodesAndNamesT::iterator it = outLanguages.begin(); it != outLanguages.end(); ++it)
      result.push_back(*it);

    result.swap(outLanguages);
    CHECK_EQUAL(outLanguages.size(), MAX_SUPPORTED_LANGUAGES, ());
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

  void GetCurrentSettings(CodesAndNamesT & outLanguages)
  {
    string settingsString;
    if (!Settings::Get(SETTING_LANG_KEY, settingsString))
      settingsString = DEFAULT_LANGUAGES; // @TODO get preffered languages from the system

    CodesT currentCodes;
    Collector c(currentCodes);
    utils::TokenizeString(settingsString, LANG_DELIMETER, c);

    GetSupportedLanguages(outLanguages);
    Sort(currentCodes, outLanguages);
  }

  void SaveSettings(CodesT const & langs)
  {
    CHECK_EQUAL(langs.size(), MAX_SUPPORTED_LANGUAGES, ());
    string const saveString = JoinStrings(langs.begin(), langs.end(), LANG_DELIMETER);
    Settings::Set(SETTING_LANG_KEY, saveString);
  }

  bool GetSupportedLanguages(CodesAndNamesT & outLanguages)
  {
    outLanguages.clear();
    FileReader file(GetPlatform().ReadPathForFile(LANGUAGES_FILE));
    string const langs = file.ReadAsText();
    istringstream stream(langs);
    for (size_t i = 0; i < MAX_SUPPORTED_LANGUAGES; ++i)
    {
      string line;
      getline(stream, line);
      size_t delimIndex = string::npos;
      if ((delimIndex = line.find(LANG_DELIMETER)) != string::npos)
        outLanguages.push_back(make_pair(line.substr(0, delimIndex), line.substr(delimIndex + 1)));
    }
    return !outLanguages.empty();
  }
}
