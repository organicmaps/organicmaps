#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <map>
#include <string>
#include <vector>

namespace feature_metadata_test
{
using namespace std;
using feature::Metadata;
using EType = Metadata::EType;

map<EType, string> const kKeyValues =
{
  {EType::FMD_ELE, "12345"},
  {EType::FMD_EMAIL, "cool@email.at"},
  // This string is longer than 255 bytes.
  {EType::FMD_WEBSITE, "http://rskxmkjwnikfnjqhyvkpjgaghhyhukjyenduiuanxgbmndtlpfphdgaizfcpzuiuspcp"
                       "umeojwvekvjprlutwjmxudyzrlwwsepewevsuqelobqcfdzsoqozkesghojribepbaitivmaqep"
                       "hheckitonddqhbapdybhetvnwvlchjafepdjaeoaapysdvculxuwjbgdddryodiihvnpvmkgqvs"
                       "mawbdsrbmnndcozmrgeoahbkhcevxkmtdqnxpxlsju.org"}
};

UNIT_TEST(Feature_Metadata_GetSet)
{
  Metadata m;
  EType const type = EType::FMD_ELE;

  // Absent types should return empty values.
  TEST_EQUAL(m.Get(type), "", ());
  m.Set(type, "12345");
  TEST_EQUAL(m.Get(type), "12345", ());
  TEST_EQUAL(m.Size(), 1, ());

  // Same types should replace old metadata values.
  m.Set(type, "9876543210");
  TEST_EQUAL(m.Get(type), "9876543210", ());

  // Empty values should drop fields.
  m.Set(type, "");
  TEST_EQUAL(m.Get(type), "", ());
  TEST_EQUAL(m.Size(), 0, ());
  TEST(m.Empty(), ());
}

UNIT_TEST(Feature_Metadata_PresentTypes)
{
  Metadata m;
  for (auto const & value : kKeyValues)
    m.Set(value.first, value.second);
  TEST_EQUAL(m.Size(), kKeyValues.size(), ());

  m.ForEach([&](Metadata::EType type, std::string const &)
  {
    TEST_EQUAL(m.Get(type), kKeyValues.find(type)->second, ());
  });
}

UNIT_TEST(Feature_Metadata_MwmTmpSerialization)
{
  Metadata original;
  for (auto const & value : kKeyValues)
    original.Set(value.first, value.second);
  TEST_EQUAL(original.Size(), kKeyValues.size(), ());

  {
    Metadata serialized;
    vector<char> buffer;
    MemWriter<decltype(buffer)> writer(buffer);
    original.SerializeForMwmTmp(writer);

    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> src(reader);
    serialized.DeserializeFromMwmTmp(src);

    for (auto const & value : kKeyValues)
      TEST_EQUAL(serialized.Get(value.first), value.second, ());
    TEST_EQUAL(serialized.Get(EType::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kKeyValues.size(), ());
  }
}

UNIT_TEST(Feature_Metadata_GetWikipedia)
{
  Metadata m;
  EType const wikiType = EType::FMD_WIKIPEDIA;
  m.Set(wikiType, "en:Article");
  TEST_EQUAL(m.Get(wikiType), "en:Article", ());
#ifdef OMIM_OS_MOBILE
  TEST_EQUAL(m.GetWikiURL(), "https://en.m.wikipedia.org/wiki/Article", ());
#else
  TEST_EQUAL(m.GetWikiURL(), "https://en.wikipedia.org/wiki/Article", ());
#endif
}

UNIT_TEST(Feature_Metadata_RegionData_Languages)
{
  {
    feature::RegionData rd;
    vector<string> const langs = {"ru", "en", "et"};
    rd.SetLanguages(langs);
    TEST(rd.HasLanguage(StringUtf8Multilang::GetLangIndex("ru")), ());
    TEST(rd.HasLanguage(StringUtf8Multilang::GetLangIndex("en")), ());
    TEST(rd.HasLanguage(StringUtf8Multilang::GetLangIndex("et")), ());
    TEST(!rd.HasLanguage(StringUtf8Multilang::GetLangIndex("es")), ());
    TEST(!rd.IsSingleLanguage(StringUtf8Multilang::GetLangIndex("ru")), ());
  }
  {
    feature::RegionData rd;
    vector<string> const langs = {"et"};
    rd.SetLanguages(langs);
    TEST(rd.HasLanguage(StringUtf8Multilang::GetLangIndex("et")), ());
    TEST(rd.IsSingleLanguage(StringUtf8Multilang::GetLangIndex("et")), ());
    TEST(!rd.HasLanguage(StringUtf8Multilang::GetLangIndex("en")), ());
    TEST(!rd.IsSingleLanguage(StringUtf8Multilang::GetLangIndex("en")), ());
  }
}

UNIT_TEST(Feature_Metadata_Print)
{
  StringUtf8Multilang s;
  s.AddString("en", "English");
  s.AddString("be", "Беларуская");

  Metadata m;
  m.Set(EType::FMD_DESCRIPTION, s.GetBuffer());

  TEST_EQUAL(DebugPrint(m), "Metadata [description=" + DebugPrint(s) + "]", ());
}
} // namespace feature_metadata_test
