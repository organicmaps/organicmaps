#include "preferred_languages.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"

#include "../std/target_os.hpp"
#include "../std/set.hpp"

#if defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  #include <CoreFoundation/CFLocale.h>
  #include <CoreFoundation/CFString.h>

#elif defined(OMIM_OS_WINDOWS)
  // @TODO

#elif defined(OMIM_OS_LINUX)
  #include "../std/stdlib.hpp"

#else
  #error "Define language preferences for your platform"

#endif

namespace languages
{

class LangFilter
{
  set<string> & m_known;
public:
  LangFilter(set<string> & known) : m_known(known) {}
  bool operator()(string const & t)
  {
    return !m_known.insert(t).second;
  }
};

class NormalizeFilter
{
public:
  void operator()(string & t)
  {
    strings::SimpleTokenizer const iter(t, "-_ ");
    if (iter)
      t = *iter;
    else
    {
      LOG(LWARNING, ("Invalid language"));
    }
  }
};

void FilterLanguages(vector<string> & langs)
{
  // normalize languages: en-US -> en, ru_RU -> ru etc.
  for_each(langs.begin(), langs.end(), NormalizeFilter());
  { // tmp storage
    set<string> known;
    // remove duplicate languages
    langs.erase(remove_if(langs.begin(), langs.end(), LangFilter(known)), langs.end());
  }
}

void SystemPreferredLanguages(vector<string> & languages)
{
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_IPHONE)
  // Mac and iOS implementation
  CFArrayRef langs = CFLocaleCopyPreferredLanguages();
  char buf[30];
  for (CFIndex i = 0; i < CFArrayGetCount(langs); ++i)
  {
    CFStringRef strRef = (CFStringRef)CFArrayGetValueAtIndex(langs, i);
    CFStringGetCString(strRef, buf, 30, kCFStringEncodingUTF8);
    languages.push_back(buf);
  }
  CFRelease(langs);

#elif defined(OMIM_OS_WINDOWS)
  // @TODO Windows implementation

#elif defined(OMIM_OS_LINUX)
  // check environment variables
  char const * p = getenv("LANGUAGE");
  if (p) // LANGUAGE can contain several values divided by ':'
  {
    string const str(p);
    strings::SimpleTokenizer iter(str, ":");
    while (iter)
    {
      languages.push_back(*iter);
      ++iter;
    }
  }
  else if ((p = getenv("LC_ALL")))
    languages.push_back(p);
  else if ((p = getenv("LC_MESSAGES")))
    languages.push_back(p);
  else if ((p = getenv("LANG")))
    languages.push_back(p);

#else
  #error "Define language preferences for your platform"
#endif

  FilterLanguages(languages);
}

string PreferredLanguages()
{
  vector<string> arr;

  SystemPreferredLanguages(arr);

  // generate output string
  string result;
  for (size_t i = 0; i < arr.size(); ++i)
  {
    result.append(arr[i]);
    result.push_back('|');
  }
  if (result.empty())
    result = "default";
  else
    result.resize(result.size() - 1);
  return result;
}

string CurrentLanguage()
{
  vector<string> arr;
  SystemPreferredLanguages(arr);
  if (arr.empty())
    return "en";
  else
    return arr[0];
}

}
