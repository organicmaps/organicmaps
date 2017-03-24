#include "coding/transliteration.hpp"
#include "coding/multilang_utf8_string.hpp"

#include "base/logging.hpp"

#include "3party/icu/common/unicode/unistr.h"
#include "3party/icu/common/unicode/utypes.h"
#include "3party/icu/i18n/unicode/translit.h"
#include "3party/icu/i18n/unicode/utrans.h"

Transliteration::~Transliteration()
{
  //u_cleanup();
}

Transliteration & Transliteration::GetInstance()
{
  static Transliteration instance;
  return instance;
}

void Transliteration::Init(std::string const & icuDataDir)
{
  u_setDataDirectory(icuDataDir.c_str());

  for (auto const & lang : StringUtf8Multilang::GetSupportedLanguages())
  {
    if (strlen(lang.m_transliteratorId) == 0 || m_transliterators.count(lang.m_transliteratorId) != 0)
      continue;

    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<Transliterator> transliterator(
          Transliterator::createInstance(lang.m_transliteratorId, UTRANS_FORWARD, status));

    if (transliterator != nullptr)
       m_transliterators.emplace(lang.m_transliteratorId, std::move(transliterator));
    else
      LOG(LWARNING, ("Cannot create transliterator \"", lang.m_transliteratorId, "\", icu error =", status));
  }
}

std::string Transliteration::Transliterate(std::string const & str, int8_t langCode) const
{
  auto const transliteratorId = StringUtf8Multilang::GetTransliteratorIdByCode(langCode);
  auto const & it = m_transliterators.find(transliteratorId);
  if (it == m_transliterators.end())
  {
    LOG(LWARNING, ("Transliteration failed, unknown transliterator \"", transliteratorId, "\""));
    return "";
  }

  UnicodeString ustr(str.c_str());
  it->second->transliterate(ustr);

  std::string resultStr;
  ustr.toUTF8String(resultStr);

  return resultStr;
}
