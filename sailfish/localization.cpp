#include "platform/localization.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <unordered_map>

namespace platform
{
namespace
{
using Strings = std::unordered_map<std::string, std::string>;

// Stops a ref cycle.
int constexpr kMaxRefDepth = 8;

// Translations of one key in decreasing priority: current language, its base language, English.
struct Entry
{
  std::string m_values[3];
  std::string m_ref;
};

// Twine values keep line breaks escaped as "\n".
std::string Unescape(std::string_view value)
{
  std::string result;
  result.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] == '\\' && i + 1 < value.size() && value[i + 1] == 'n')
    {
      result += '\n';
      ++i;
    }
    else
    {
      result += value[i];
    }
  }
  return result;
}

// Plural forms (lang:other) are skipped, they are not used through this API.
Strings LoadStrings()
{
  std::string const lang = languages::GetCurrentTwine();
  std::string const baseLang = lang.substr(0, lang.find('-'));

  std::unordered_map<std::string, Entry> entries;
  for (char const * file : {"strings/types_strings.txt", "strings/strings.txt", "strings/sailfish_strings.txt"})
  {
    std::ifstream in(GetPlatform().ResourcesDir() + file);
    if (!in)
    {
      LOG(LWARNING, ("Can't open", file));
      continue;
    }

    Entry * entry = nullptr;
    std::string line;
    while (std::getline(in, line))
    {
      std::string_view trimmed = line;
      strings::Trim(trimmed);
      if (trimmed.size() > 2 && trimmed.front() == '[' && trimmed.back() == ']')
      {
        // [[Section]] headers are not keys.
        entry = trimmed[1] == '[' ? nullptr : &entries[std::string(trimmed.substr(1, trimmed.size() - 2))];
        continue;
      }

      auto const eq = trimmed.find(" = ");
      if (!entry || eq == std::string_view::npos)
        continue;
      std::string_view const code = trimmed.substr(0, eq);
      std::string_view const value = trimmed.substr(eq + 3);
      if (code == lang)
        entry->m_values[0] = value;
      else if (code == baseLang)
        entry->m_values[1] = value;
      else if (code == "en")
        entry->m_values[2] = value;
      else if (code == "ref")
        entry->m_ref = value;
    }
  }

  // A ref supplies the translations an entry lacks, so look for each priority along the ref chain
  // before falling back to the next one.
  Strings strings;
  for (auto const & [key, entry] : entries)
  {
    for (int const value : {0, 1, 2})
    {
      Entry const * e = &entry;
      for (int depth = 0; e && e->m_values[value].empty() && !e->m_ref.empty() && depth < kMaxRefDepth; ++depth)
      {
        auto const it = entries.find(e->m_ref);
        e = it != entries.end() ? &it->second : nullptr;
      }
      if (e && !e->m_values[value].empty())
      {
        strings.emplace(key, Unescape(e->m_values[value]));
        break;
      }
    }
  }
  LOG(LINFO, ("Loaded", strings.size(), "strings for", lang));
  return strings;
}

std::string GetString(std::string const & key)
{
  static Strings const strings = LoadStrings();
  auto const it = strings.find(key);
  return it != strings.end() ? it->second : key;
}
}  // namespace

std::string GetLocalizedTypeName(std::string const & type)
{
  // The key mapping of the iOS and Android string tables.
  auto key = "type." + type;
  std::replace(key.begin(), key.end(), '-', '.');
  std::replace(key.begin(), key.end(), ':', '_');
  return GetString(key);
}

std::string GetLocalizedString(std::string const & key)
{
  return GetString(key);
}
}  // namespace platform
