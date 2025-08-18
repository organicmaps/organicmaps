#pragma once

#include "coding/string_utf8_multilang.hpp"

#include <deque>
#include <functional>
#include <istream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>

class Reader;

namespace indexer
{
class BrandsHolder
{
public:
  struct Brand
  {
    struct Name
    {
      Name(std::string_view name, int8_t locale) : m_name(name), m_locale(locale) {}

      bool operator==(Name const & rhs) const;
      bool operator<(Name const & rhs) const;

      std::string m_name;
      // Same with StringUtf8Multilang locales.
      int8_t m_locale;
    };

    void Swap(Brand & r) { m_synonyms.swap(r.m_synonyms); }

    std::deque<Name> m_synonyms;
  };

  explicit BrandsHolder(std::unique_ptr<Reader> && reader);

  std::set<std::string> const & GetKeys() const { return m_keys; }

  template <class FnT>
  void ForEachNameByKey(std::string_view key, FnT && fn) const
  {
    auto const it = m_keyToName.find(key);
    if (it == m_keyToName.end())
      return;

    for (auto const & name : it->second->m_synonyms)
      fn(name);
  }

  void ForEachNameByKeyAndLang(std::string const & key, std::string const & lang,
                               std::function<void(std::string const &)> const & toDo) const;

private:
  void LoadFromStream(std::istream & s);
  void AddBrand(Brand & brand, std::string const & key);

  struct StringViewHash
  {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    // std::size_t operator()(const char* str) const        { return hash_type{}(str); }
    size_t operator()(std::string_view str) const { return hash_type{}(str); }
    size_t operator()(std::string const & str) const { return hash_type{}(str); }
  };

  std::unordered_map<std::string, std::shared_ptr<Brand>, StringViewHash, std::equal_to<>> m_keyToName;
  std::set<std::string> m_keys;
};

std::string DebugPrint(BrandsHolder::Brand::Name const & name);
BrandsHolder const & GetDefaultBrands();

template <class FnT>
void ForEachLocalizedBrands(std::string_view brand, FnT && fn)
{
  bool processed = false;
  /// Localized brands are not working as expected now because we store raw names from OSM, not brand IDs.
  GetDefaultBrands().ForEachNameByKey(brand, [&fn, &processed](auto const & name)
  {
    fn(name);
    processed = true;
  });

  if (!processed)
    fn(BrandsHolder::Brand::Name(brand, StringUtf8Multilang::kDefaultCode));
}

}  // namespace indexer
