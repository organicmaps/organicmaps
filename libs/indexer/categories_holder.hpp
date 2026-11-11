#pragma once

#include "base/mem_trie.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Reader;

class CategoriesHolder
{
public:
  struct Category
  {
    static constexpr uint8_t kEmptyPrefixLength = 10;

    struct Name
    {
      std::string m_name;
      /// This language/locale code is completely different from our built-in langs in
      /// string_utf8_multilang.cpp and is only used for mapping user's input language to our values
      /// in categories.txt file
      int8_t m_locale;
      uint8_t m_prefixLengthToSuggest;
    };

    std::vector<Name> m_synonyms;

    void Swap(Category & r) { m_synonyms.swap(r.m_synonyms); }
  };

  using GroupTranslations = std::unordered_map<std::string, std::vector<Category::Name>>;

private:
  using Type2CategoryCont = std::multimap<uint32_t, std::shared_ptr<Category>>;
  using Trie = base::MemTrie<strings::UniString, base::VectorValues<uint32_t>>;

  Type2CategoryCont m_type2cat;

  // Maps locale and category token to the list of corresponding types.
  // Locale is treated as a special symbol prepended to the token.
  Trie m_name2type;

  GroupTranslations m_groupTranslations;

public:
  static int8_t constexpr kUnsupportedLocaleCode = -1;
  // Codes are 1-based positions used only within the process. Keep English first and place variants
  // after their base language so the reverse scan prefers them. Named codes are derived from this table.
  static auto constexpr kLocales = std::to_array<std::string_view>({
      "en", "en-AU", "en-GB", "en-US", "ar", "be", "bg", "ca", "cs", "cy", "da", "de", "el", "es", "es-MX",   "et",
      "eu", "fa",    "fi",    "fr",    "he", "hi", "hu", "id", "it", "ja", "ko", "lt", "lv", "mr", "nb",      "nl",
      "pl", "pt",    "pt-BR", "ro",    "ru", "sk", "sl", "sr", "sv", "sw", "th", "tr", "uk", "vi", "zh-Hans", "zh-Hant",
  });

  static_assert(kLocales.size() <= std::numeric_limits<int8_t>::max(), "Category locale codes must fit in int8_t");
  static_assert([]
  {
    for (size_t i = 0; i < kLocales.size(); ++i)
      for (size_t j = i + 1; j < kLocales.size(); ++j)
        if (kLocales[i] == kLocales[j])
          return false;
    return true;
  }(), "Category locales must be unique");

  static int8_t constexpr kEnglishCode = std::find(kLocales.begin(), kLocales.end(), "en") - kLocales.begin() + 1;
  static int8_t constexpr kSimplifiedChineseCode =
      std::find(kLocales.begin(), kLocales.end(), "zh-Hans") - kLocales.begin() + 1;
  static int8_t constexpr kTraditionalChineseCode =
      std::find(kLocales.begin(), kLocales.end(), "zh-Hant") - kLocales.begin() + 1;

  static_assert(kEnglishCode == 1, "English must be the first category locale");
  static_assert(kSimplifiedChineseCode > 0 && kSimplifiedChineseCode <= kLocales.size(), "Missing Simplified Chinese");
  static_assert(kTraditionalChineseCode > 0 && kTraditionalChineseCode <= kLocales.size(),
                "Missing Traditional Chinese");

  explicit CategoriesHolder(std::unique_ptr<Reader> && reader);

  template <class ToDo>
  void ForEachCategory(ToDo && toDo) const
  {
    for (auto const & p : m_type2cat)
      toDo(*p.second);
  }

  template <class ToDo>
  void ForEachTypeAndCategory(ToDo && toDo) const
  {
    for (auto const & it : m_type2cat)
      toDo(it.first, *it.second);
  }

  template <class ToDo>
  void ForEachName(ToDo && toDo) const
  {
    for (auto const & p : m_type2cat)
      for (auto const & synonym : p.second->m_synonyms)
        toDo(synonym);
  }

  template <class ToDo>
  void ForEachNameAndType(ToDo && toDo) const
  {
    for (auto const & p : m_type2cat)
      for (auto const & synonym : p.second->m_synonyms)
        toDo(synonym, p.first);
  }

  template <class ToDo>
  void ForEachNameByType(uint32_t type, ToDo && toDo) const
  {
    // A type can appear in multiple category records with different synonyms.
    auto const [begin, end] = m_type2cat.equal_range(type);
    for (auto it = begin; it != end; ++it)
      for (auto const & name : it->second->m_synonyms)
        toDo(name);
  }

  template <class ToDo>
  void ForEachTypeByName(int8_t locale, strings::UniString const & name, ToDo && toDo) const
  {
    auto const localePrefix = strings::UniString(1, static_cast<strings::UniChar>(locale));
    m_name2type.ForEachInNode(localePrefix + name, toDo);
  }

  GroupTranslations const & GetGroupTranslations() const { return m_groupTranslations; }

  /// Search name for type with preffered locale language.
  /// If no name for this language, return en name.
  /// @return false if no categories for type.
  bool GetNameByType(uint32_t type, int8_t locale, std::string & name) const;

  /// @returns raw classificator type if it's not localized in categories.txt.
  std::string GetReadableFeatureType(uint32_t type, int8_t locale) const;

  // Exposes the tries that map category tokens to types.
  Trie const & GetNameToTypesTrie() const { return m_name2type; }
  bool IsTypeExist(uint32_t type) const;

  void Swap(CategoriesHolder & r)
  {
    m_type2cat.swap(r.m_type2cat);
    std::swap(m_name2type, r.m_name2type);
  }

  // Converts any language |locale| from UI to the corresponding
  // internal integer code.
  static int8_t MapLocaleToInteger(std::string_view const locale);

  // Returns corresponding string representation for an internal
  // integer |code|. Returns an empty string in case of invalid
  // |code|.
  static std::string MapIntegerToLocale(int8_t code);

private:
  void LoadFromStream(std::istream & s);
  void AddCategory(Category & cat, std::vector<uint32_t> & types);
  static bool ValidKeyToken(strings::UniString const & s);
};

inline void swap(CategoriesHolder & a, CategoriesHolder & b)
{
  return a.Swap(b);
}

// Defined in categories_holder_loader.cpp.
CategoriesHolder const & GetDefaultCategories();
CategoriesHolder const & GetDefaultCuisineCategories();
