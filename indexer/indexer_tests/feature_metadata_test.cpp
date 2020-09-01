#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/target_os.hpp"

#include <map>
#include <string>
#include <vector>

using namespace std;
using feature::Metadata;

namespace
{
map<Metadata::EType, string> const kKeyValues =
{
  {Metadata::FMD_ELE, "12345"},
  {Metadata::FMD_CUISINE, "greek;mediterranean"},
  {Metadata::FMD_EMAIL, "cool@email.at"},
  // This string is longer than 255 bytes.
  {Metadata::FMD_URL, "http://rskxmkjwnikfnjqhyv"
                      "kpjgaghhyhukjyenduiuanxgb"
                      "mndtlpfphdgaizfcpzuiuspcp"
                      "umeojwvekvjprlutwjmxudyzr"
                      "lwwsepewevsuqelobqcfdzsoq"
                      "ozkesghojribepbaitivmaqep"
                      "hheckitonddqhbapdybhetvnw"
                      "vlchjafepdjaeoaapysdvculx"
                      "uwjbgdddryodiihvnpvmkgqvs"
                      "mawbdsrbmnndcozmrgeoahbkh"
                      "cevxkmtdqnxpxlsju.org"}
};
} // namespace

UNIT_TEST(Feature_Metadata_GetSet)
{
  Metadata m;
  Metadata::EType const type = Metadata::FMD_ELE;
  // Absent types should return empty values.
  TEST_EQUAL(m.Get(type), "", ());
  m.Set(type, "12345");
  TEST_EQUAL(m.Get(type), "12345", ());
  TEST_EQUAL(m.Size(), 1, ());
  // Same types should replace old metadata values.
  m.Set(type, "5678");
  TEST_EQUAL(m.Get(type), "5678", ());
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

  auto const types = m.GetPresentTypes();
  TEST_EQUAL(types.size(), m.Size(), ());
  for (auto const & type : types)
    TEST_EQUAL(m.Get(type), kKeyValues.find(static_cast<Metadata::EType>(type))->second, ());
}

UNIT_TEST(Feature_MwmTmpSerialization)
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
    TEST_EQUAL(serialized.Get(Metadata::FMD_OPERATOR), "", ());
    TEST_EQUAL(serialized.Size(), kKeyValues.size(), ());
  }
}

UNIT_TEST(Feature_Metadata_GetWikipedia)
{
  Metadata m;
  Metadata::EType const wikiType = Metadata::FMD_WIKIPEDIA;
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
