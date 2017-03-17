#include "search/search_quality/assessment_tool/languages_list.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"

#include "coding/multilang_utf8_string.hpp"

LanguagesList::LanguagesList(QWidget * parent) : QComboBox(parent)
{
  auto const langs = StringUtf8Multilang::GetSupportedLanguages();
  for (auto const & lang : langs)
    addItem(ToQString(lang.m_code));
}

void LanguagesList::Select(std::string const & lang)
{
  auto const index = StringUtf8Multilang::GetLangIndex(lang);
  if (index != StringUtf8Multilang::kUnsupportedLanguageCode)
    setCurrentIndex(index);
  else
    setCurrentIndex(StringUtf8Multilang::kDefaultCode);
}
