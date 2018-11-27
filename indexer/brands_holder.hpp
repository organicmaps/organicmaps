#pragma once

#include <cstdint>
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
      Name(std::string name, int8_t locale) : m_name(name), m_locale(locale) {}

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

  void ForEachNameByKey(std::string const & key,
                        std::function<void(Brand::Name const &)> const & toDo) const;

  void ForEachNameByKeyAndLang(std::string const & key, std::string const & lang,
                               std::function<void(std::string const &)> const & toDo) const;

private:
  void LoadFromStream(std::istream & s);
  void AddBrand(Brand & brand, std::string const & key);

  std::unordered_map<std::string, std::shared_ptr<Brand>> m_keyToName;
  std::set<std::string> m_keys;
};

std::string DebugPrint(BrandsHolder::Brand::Name const & name);
BrandsHolder const & GetDefaultBrands();
}  // namespace indexer
