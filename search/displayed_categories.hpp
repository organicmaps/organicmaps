#pragma once

#include "indexer/categories_holder.hpp"

#include <string>
#include <vector>

namespace search
{
class CategoriesModifier;
// *NOTE* This class is not thread-safe.
class DisplayedCategories
{
public:
  using Keys = std::vector<std::string>;

  DisplayedCategories(CategoriesHolder const & holder);

  void Modify(CategoriesModifier & modifier);

  // Returns a list of English names of displayed categories for the categories search tab.
  // The list may be modified during the application runtime in order to support sponsored or
  // featured categories. Keys may be used as parts of resources ids.
  Keys const & GetKeys() const;

  // Calls |fn| on each pair (synonym name, synonym locale) for the
  // |key|.
  template <typename Fn>
  void ForEachSynonym(std::string const & key, Fn && fn) const
  {
    auto const & translations = m_holder.GetGroupTranslations();
    auto const it = translations.find("@" + key);
    if (it == translations.end())
      return;

    for (auto const & name : it->second)
      fn(name.m_name, CategoriesHolder::MapIntegerToLocale(name.m_locale));
  }

  static bool IsLanguageSupported(std::string_view locale)
  {
    return CategoriesHolder::MapLocaleToInteger(locale) != CategoriesHolder::kUnsupportedLocaleCode;
  }

 private:
  CategoriesHolder const & m_holder;
  Keys m_keys;
};

class CategoriesModifier
{
public:
  virtual ~CategoriesModifier() = default;

  virtual void Modify(DisplayedCategories::Keys & keys) = 0;
};
}  // namespace search
