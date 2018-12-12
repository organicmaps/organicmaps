#include "indexer/brands_holder.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/logging.hpp"

#include <sstream>
#include <utility>

#include "defines.hpp"

using namespace std;

namespace
{
enum class State
{
  ParseBrand,
  ParseLanguages
};
}  // namespace

namespace indexer
{
bool BrandsHolder::Brand::Name::operator==(BrandsHolder::Brand::Name const & rhs) const
{
  return m_locale == rhs.m_locale && m_name == rhs.m_name;
}

bool BrandsHolder::Brand::Name::operator<(BrandsHolder::Brand::Name const & rhs) const
{
  if (m_locale != rhs.m_locale)
    return m_locale < rhs.m_locale;
  return m_name < rhs.m_name;
}

BrandsHolder::BrandsHolder(unique_ptr<Reader> && reader)
{
  ReaderStreamBuf buffer(move(reader));
  istream s(&buffer);
  LoadFromStream(s);
}

void BrandsHolder::ForEachNameByKey(std::string const & key,
                                    std::function<void(Brand::Name const &)> const & toDo) const
{
  auto const it = m_keyToName.find(key);
  if (it == m_keyToName.end())
    return;

  for (auto const & name : it->second->m_synonyms)
    toDo(name);
}

void BrandsHolder::ForEachNameByKeyAndLang(std::string const & key, std::string const & lang,
                                           std::function<void(std::string const &)> const & toDo) const
{
  int8_t const locale = StringUtf8Multilang::GetLangIndex(lang);
  ForEachNameByKey(key, [&](Brand::Name const & name) {
    if (name.m_locale == locale)
      toDo(name.m_name);
  });
}

void BrandsHolder::LoadFromStream(istream & s)
{
  m_keys.clear();
  m_keyToName.clear();

  State state = State::ParseBrand;
  string line;
  Brand brand;
  string key;

  char const kKeyPrefix[] = "brand.";
  size_t const kKeyPrefixLength = strlen(kKeyPrefix);

  int lineNumber = 0;
  while (s.good())
  {
    ++lineNumber;
    getline(s, line);
    strings::Trim(line);
    // Allow comments starting with '#' character.
    if (!line.empty() && line[0] == '#')
      continue;

    strings::SimpleTokenizer iter(line, state == State::ParseBrand ? "|" : ":|");

    if (state == State::ParseBrand)
    {
      if (!iter)
        continue;

      AddBrand(brand, key);

      CHECK_GREATER((*iter).size(), kKeyPrefixLength, (lineNumber, *iter));
      ASSERT_EQUAL((*iter).find(kKeyPrefix), 0, (lineNumber, *iter));
      key = (*iter).substr(kKeyPrefixLength);
      CHECK(!++iter, (lineNumber));
      state = State::ParseLanguages;
    }
    else if (state == State::ParseLanguages)
    {
      if (!iter)
      {
        state = State::ParseBrand;
        continue;
      }

      int8_t const langCode = StringUtf8Multilang::GetLangIndex(*iter);
      CHECK_NOT_EQUAL(langCode, StringUtf8Multilang::kUnsupportedLanguageCode, ());

      while (++iter)
      {
        Brand::Name name(*iter, langCode);

        if (!name.m_name.empty())
          brand.m_synonyms.push_back(name);
      }
    }
  }

  // Add last brand.
  AddBrand(brand, key);
}

void BrandsHolder::AddBrand(Brand & brand, string const & key)
{
  if (key.empty())
    return;

  CHECK(!brand.m_synonyms.empty(), ());

  shared_ptr<Brand> p(new Brand());
  p->Swap(brand);

  m_keyToName.emplace(key, p);
  CHECK(m_keys.insert(key).second, ("Key duplicate", key));
}

string DebugPrint(BrandsHolder::Brand::Name const & name)
{
  std::ostringstream out;
  out << "BrandName[" << StringUtf8Multilang::GetLangByCode(name.m_locale) << ", " << name.m_name
      << "]";
  return out.str();
}

BrandsHolder const & GetDefaultBrands()
{
  static BrandsHolder const instance(GetPlatform().GetReader(SEARCH_BRAND_CATEGORIES_FILE_NAME));
  return instance;
}
}  // namespace indexer
