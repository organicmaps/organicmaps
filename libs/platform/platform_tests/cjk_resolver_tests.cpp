#include "testing/testing.hpp"

#include "platform/preferred_languages.hpp"

#include <set>

using languages::CJKResolver;
using V = CJKResolver::Variant;

UNIT_TEST(CJKResolver_FromLanguageTag)
{
  // Japanese.
  TEST_EQUAL(CJKResolver::FromLanguageTag("ja"), V::JP, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ja-JP"), V::JP, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ja_JP"), V::JP, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("JA"), V::JP, ());

  // Korean.
  TEST_EQUAL(CJKResolver::FromLanguageTag("ko"), V::KR, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ko-KR"), V::KR, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ko_KR"), V::KR, ());

  // Simplified Chinese.
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Hans"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-CN"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh_CN"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Hans-CN"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-SG"), V::SC, ());

  // Traditional Chinese.
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Hant"), V::TC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-TW"), V::TC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh_TW"), V::TC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Hant-TW"), V::TC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh_TW_#Hant"), V::TC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-MO"), V::TC, ());

  // Hong Kong.
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-HK"), V::HK, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh_HK"), V::HK, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Hant-HK"), V::HK, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ZH-HK"), V::HK, ());

  // Non-CJK locales default to SC so a Han glyph is at least rendered.
  TEST_EQUAL(CJKResolver::FromLanguageTag("en"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("en-US"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("ru-RU"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag(""), V::SC, ());

  // Tags whose primary subtag merely starts with "ja"/"ko"/"zh" must not be misclassified.
  TEST_EQUAL(CJKResolver::FromLanguageTag("jav"), V::SC, ());  // Javanese
  TEST_EQUAL(CJKResolver::FromLanguageTag("jav-ID"), V::SC, ());
  TEST_EQUAL(CJKResolver::FromLanguageTag("jam"), V::SC, ());       // Jamaican Creole
  TEST_EQUAL(CJKResolver::FromLanguageTag("kos"), V::SC, ());       // Kosraean
  TEST_EQUAL(CJKResolver::FromLanguageTag("zhongwen"), V::SC, ());  // not a real tag, pins primary-subtag matching
  TEST_EQUAL(CJKResolver::FromLanguageTag("zh-Mong"), V::SC, ());   // "mo" substring is not the MO region subtag
}

UNIT_TEST(CJKResolver_FromSfntFamilyName)
{
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK JP"), std::optional{V::JP}, ());
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK KR"), std::optional{V::KR}, ());
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK SC"), std::optional{V::SC}, ());
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK TC"), std::optional{V::TC}, ());
  // HK must take priority over substring matches against TC/SC.
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK HK"), std::optional{V::HK}, ());
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK HK TC"), std::optional{V::HK}, ());
  TEST_EQUAL(CJKResolver::FromSfntFamilyName("Noto Sans CJK HK SC"), std::optional{V::HK}, ());

  TEST(!CJKResolver::FromSfntFamilyName("Noto Sans"), ());
  TEST(!CJKResolver::FromSfntFamilyName("Roboto"), ());
  TEST(!CJKResolver::FromSfntFamilyName(""), ());
}

UNIT_TEST(CJKResolver_FromFontFileName)
{
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansJP-Regular.otf"), std::optional{V::JP}, ());
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansKR-Regular.otf"), std::optional{V::KR}, ());
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansSC-Regular.otf"), std::optional{V::SC}, ());
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansTC-Regular.otf"), std::optional{V::TC}, ());
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansHK-Regular.otf"), std::optional{V::HK}, ());

  // Older Noto naming for SC/TC.
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansHans-Regular.ttf"), std::optional{V::SC}, ());
  TEST_EQUAL(CJKResolver::FromFontFileName("NotoSansHant-Regular.ttf"), std::optional{V::TC}, ());

  TEST(!CJKResolver::FromFontFileName("Roboto-Regular.ttf"), ());
  TEST(!CJKResolver::FromFontFileName("NotoNaskhArabic-Regular.ttf"), ());
  TEST(!CJKResolver::FromFontFileName("NotoSansSymbols-Regular.ttf"), ());
  TEST(!CJKResolver::FromFontFileName(""), ());
}

UNIT_TEST(CJKResolver_IsCJKContainerFileName)
{
  TEST(CJKResolver::IsCJKContainerFileName("NotoSansCJK-Regular.ttc"), ());
  TEST(CJKResolver::IsCJKContainerFileName("/system/fonts/NotoSansCJK-Regular.ttc"), ());

  TEST(!CJKResolver::IsCJKContainerFileName("NotoSansSC-Regular.otf"), ());
  TEST(!CJKResolver::IsCJKContainerFileName("Roboto-Regular.ttf"), ());
  TEST(!CJKResolver::IsCJKContainerFileName(""), ());
  // Pin case-sensitivity so the contract is explicit if a future filesystem returns lowercase.
  TEST(!CJKResolver::IsCJKContainerFileName("notosanscjk-regular.ttc"), ());
  // Suffix gate: the substring alone must not classify a non-collection file as a TTC.
  TEST(!CJKResolver::IsCJKContainerFileName("NotoSansCJK-Regular.otf"), ());
  TEST(!CJKResolver::IsCJKContainerFileName("NotoSansCJK.txt"), ());
}

UNIT_TEST(CJKResolver_FallbackChain)
{
  // First entry must be the variant itself, and the chain must be exhaustive.
  for (auto user : {V::JP, V::KR, V::SC, V::TC, V::HK})
  {
    auto const chain = CJKResolver::FallbackChain(user);
    TEST_EQUAL(chain[0], user, ());
    std::set<V> const unique(chain.begin(), chain.end());
    TEST_EQUAL(unique.size(), 5, ("Chain must contain all 5 variants for", DebugPrint(user)));
  }

  // HK should prefer TC over SC; both come before JP/KR.
  auto const hk = CJKResolver::FallbackChain(V::HK);
  TEST_EQUAL(hk[1], V::TC, ());
  TEST_EQUAL(hk[2], V::SC, ());

  // JP/KR users prefer SC over the other CJ family.
  TEST_EQUAL(CJKResolver::FallbackChain(V::JP)[1], V::SC, ());
  TEST_EQUAL(CJKResolver::FallbackChain(V::KR)[1], V::SC, ());
}

UNIT_TEST(CJKResolver_DefaultCtor_Smoke)
{
  CJKResolver const r;
  auto const v = r.User();
  TEST(v == V::JP || v == V::KR || v == V::SC || v == V::TC || v == V::HK, ());
  TEST_EQUAL(r.FallbackChain(), CJKResolver::FallbackChain(v), ());
}
