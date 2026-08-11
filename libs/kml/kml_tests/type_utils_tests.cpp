#include "testing/testing.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace type_utils_tests
{
using kml::GetPreferredBookmarkName;
using kml::GetPreferredBookmarkStr;

int8_t const kEn = StringUtf8Multilang::kEnglishCode;
int8_t const kInt = StringUtf8Multilang::kInternationalCode;
int8_t const kDe = StringUtf8Multilang::GetLangIndex("de");
int8_t const kRu = StringUtf8Multilang::GetLangIndex("ru");

UNIT_TEST(GetPreferredBookmarkStr_MatchingLanguage)
{
  kml::LocalizableString const name{{kRu, "Эрмитаж"}, {kDe, "Eremitage"}};

  // The map language always wins when it is present.
  TEST_EQUAL(GetPreferredBookmarkStr(name, "ru"), "Эрмитаж", ());
  TEST_EQUAL(GetPreferredBookmarkStr(name, "de"), "Eremitage", ());

  // So do the languages every device understands, before the last-resort pick below.
  kml::LocalizableString withEn = name;
  withEn[kEn] = "Hermitage";
  TEST_EQUAL(GetPreferredBookmarkStr(withEn, "fr"), "Hermitage", ());
}

UNIT_TEST(GetPreferredBookmarkStr_NoMatchingLanguage)
{
  // A single translation is shown as is, whatever its language.
  TEST_EQUAL(GetPreferredBookmarkStr({{kRu, "Эрмитаж"}}, "fr"), "Эрмитаж", ());

  // With no preferred-language match, the stable fallback keeps the value visible and agrees with
  // serialization.
  kml::LocalizableString const name{{kRu, "Эрмитаж"}, {kDe, "Eremitage"}};
  TEST_EQUAL(GetPreferredBookmarkStr(name, "fr"), kml::GetStringForExport(name), ());
  TEST_EQUAL(GetPreferredBookmarkStr(name, "fr"), "Eremitage", ());

  // A string with nothing in it stays empty, so the callers' own fallbacks still kick in.
  TEST_EQUAL(GetPreferredBookmarkStr({}, "fr"), "", ());
}

UNIT_TEST(GetPreferredBookmarkStr_DefaultLanguagePreservesInternationalName)
{
  kml::LocalizableString const name{{kInt, "Arabian Island, Arabia"}, {kRu, "Арабский остров"}};

  // Feature-name rendering shortens comma-separated int_name values. The device-independent
  // resolver keeps the stored bookmark string intact.
  TEST_EQUAL(GetPreferredBookmarkStr(name, "default"), kml::GetStringForExport(name), ());
  TEST_EQUAL(GetPreferredBookmarkStr(name, "default"), "Arabian Island, Arabia", ());
}

UNIT_TEST(GetPreferredBookmarkName_DefaultLanguagePreservesInternationalName)
{
  kml::BookmarkData bookmark;
  bookmark.m_name = {{kInt, "Arabian Island, Arabia"}, {kRu, "Арабский остров"}};
  TEST_EQUAL(GetPreferredBookmarkName(bookmark, "default"), "Arabian Island, Arabia", ());
}

UNIT_TEST(GetPreferredBookmarkStr_NoMatchingRegionLanguage)
{
  feature::RegionData regionData;
  regionData.SetLanguages({"uk"});
  kml::LocalizableString const name{{kRu, "Эрмитаж"}, {kDe, "Eremitage"}};

  TEST_EQUAL(GetPreferredBookmarkStr(name, regionData, "fr"), "Eremitage", ());
}

UNIT_TEST(GetPreferredBookmarkName_NoMatchingLanguage)
{
  classificator::Load();
  auto const castle = classif().GetTypeByPath({"historic", "castle"});

  // The bookmark's own name is preferred over its feature type even when no language matches.
  kml::BookmarkData named;
  named.m_name = {{kRu, "Эрмитаж"}, {kDe, "Eremitage"}};
  named.m_featureTypes = {castle};
  TEST_EQUAL(GetPreferredBookmarkName(named, "fr"), "Eremitage", ());

  // A name typed by the user still wins over the one the bookmark was created with.
  kml::BookmarkData renamed = named;
  renamed.m_customName = {{kRu, "Мой Эрмитаж"}, {kDe, "Meine Eremitage"}};
  TEST_EQUAL(GetPreferredBookmarkName(renamed, "fr"), "Meine Eremitage", ());

  // The localized feature type remains the last resort for a nameless bookmark.
  kml::BookmarkData nameless;
  nameless.m_featureTypes = {castle};
  TEST_EQUAL(GetPreferredBookmarkName(nameless, "fr"), kml::GetLocalizedFeatureType({castle}), ());

  // A default-language request uses the complete serialization fallback ladder.
  for (auto const & bookmark : {named, renamed, nameless})
    TEST_EQUAL(GetPreferredBookmarkName(bookmark, "fr"), GetPreferredBookmarkName(bookmark, "default"), ());
}
}  // namespace type_utils_tests
