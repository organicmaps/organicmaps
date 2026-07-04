#include "testing/testing.hpp"

#include "indexer/feature_meta.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <chrono>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace feature_metadata_test
{
using namespace std;
using feature::Metadata;
using EType = Metadata::EType;

map<EType, string> const kKeyValues = {{EType::FMD_ELE, "12345"},
                                       {EType::FMD_EMAIL, "cool@email.at"},
                                       // This string is longer than 255 bytes.
                                       {EType::FMD_WEBSITE,
                                        "http://rskxmkjwnikfnjqhyvkpjgaghhyhukjyenduiuanxgbmndtlpfphdgaizfcpzuiuspcp"
                                        "umeojwvekvjprlutwjmxudyzrlwwsepewevsuqelobqcfdzsoqozkesghojribepbaitivmaqep"
                                        "hheckitonddqhbapdybhetvnwvlchjafepdjaeoaapysdvculxuwjbgdddryodiihvnpvmkgqvs"
                                        "mawbdsrbmnndcozmrgeoahbkhcevxkmtdqnxpxlsju.org"}};

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
  { TEST_EQUAL(m.Get(type), kKeyValues.find(type)->second, ()); });
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

namespace
{
// year_month_day has no DebugPrint(), and ISO strings make a failure readable.
vector<string> ToStrings(feature::RegionData::PublicHolidaysT const & dates)
{
  vector<string> result;
  for (auto const & ymd : dates)
  {
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02u-%02u", int(ymd.year()), unsigned(ymd.month()), unsigned(ymd.day()));
    result.emplace_back(buf);
  }
  return result;
}

vector<string> Expand(int8_t reference, int8_t offset, int yearFrom, int yearTo)
{
  feature::RegionData rd;
  rd.AddPublicHoliday(reference, offset);
  return ToStrings(rd.GetPublicHolidays(yearFrom, yearTo));
}
}  // namespace

UNIT_TEST(Feature_Metadata_RegionData_PublicHolidays)
{
  TEST(feature::RegionData().GetPublicHolidays(2026, 2026).empty(), ());

  feature::RegionData rd;
  rd.AddPublicHoliday(1, 1);                               // January 1
  rd.AddPublicHoliday(12, 25);                             // December 25
  rd.AddPublicHoliday(feature::RegionData::PH_EASTER, 1);  // Easter Monday
  // Every holiday must survive: AddPublicHoliday() used to keep only the first one.
  TEST_EQUAL(ToStrings(rd.GetPublicHolidays(2026, 2026)), vector<string>({"2026-01-01", "2026-12-25", "2026-04-06"}),
             ());
  TEST_EQUAL(rd.GetPublicHolidays(2025, 2026).size(), size_t{6}, ());

  // Holidays survive an mwm round-trip and are expanded on Deserialize().
  vector<char> buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  rd.Serialize(writer);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  feature::RegionData loaded;
  loaded.Deserialize(src);
  TEST_EQUAL(ToStrings(loaded.GetPublicHolidays(2026, 2026)), ToStrings(rd.GetPublicHolidays(2026, 2026)), ());
  TEST_EQUAL(loaded.GetPublicHolidays().size(), size_t{12}, ());  // 3 holidays over a 4 year window.
}

UNIT_TEST(Feature_Metadata_RegionData_PublicHolidays_Floating)
{
  using RD = feature::RegionData;
  // Century boundaries exercise the Julian -> Gregorian shift of Orthodox Easter.
  TEST_EQUAL(Expand(RD::PH_EASTER, 0, 2024, 2026), vector<string>({"2024-03-31", "2025-04-20", "2026-04-05"}), ());
  TEST_EQUAL(Expand(RD::PH_EASTER, 0, 1900, 1900), vector<string>({"1900-04-15"}), ());
  TEST_EQUAL(Expand(RD::PH_EASTER, 0, 2100, 2100), vector<string>({"2100-03-28"}), ());
  TEST_EQUAL(Expand(RD::PH_EASTER, -2, 2026, 2026), vector<string>({"2026-04-03"}), ());  // Good Friday

  TEST_EQUAL(Expand(RD::PH_ORTHODOX_EASTER, 0, 2024, 2027),
             vector<string>({"2024-05-05", "2025-04-20", "2026-04-12", "2027-05-02"}), ());
  TEST_EQUAL(Expand(RD::PH_ORTHODOX_EASTER, 0, 1900, 1900), vector<string>({"1900-04-22"}), ());
  TEST_EQUAL(Expand(RD::PH_ORTHODOX_EASTER, 0, 2100, 2100), vector<string>({"2100-05-02"}), ());

  // 2020 is the year May 25 is itself a Monday, so Victoria Day steps a full week back.
  TEST_EQUAL(Expand(RD::PH_VICTORIA_DAY, 0, 2020, 2023),
             vector<string>({"2020-05-18", "2021-05-24", "2022-05-23", "2023-05-22"}), ());

  // Canada Day moves to July 2 when July 1 is a Sunday (Holidays Act).
  TEST_EQUAL(Expand(RD::PH_CANADA_DAY, 0, 2026, 2029),
             vector<string>({"2026-07-01", "2027-07-01", "2028-07-01", "2029-07-02"}), ());
}

UNIT_TEST(Feature_Metadata_RegionData_PublicHolidays_Invalid)
{
  feature::RegionData rd;
  rd.AddPublicHoliday(2, 30);  // February 30 does not exist.
  rd.AddPublicHoliday(13, 1);  // Neither a month nor a known reference.
  rd.AddPublicHoliday(1, 6);
  TEST_EQUAL(ToStrings(rd.GetPublicHolidays(2026, 2026)), vector<string>({"2026-01-06"}), ());
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
}  // namespace feature_metadata_test
