#include "../../testing/testing.hpp"

#include "../guides.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

// Needed for friend functions to work correctly
namespace guides
{

UNIT_TEST(Guides_SmokeTest)
{
  GuidesManager manager;
  GuideInfo info;

  string const str =  "{ \"version\": 2,"
                  "\"UK\": {"
                  "\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\","
                  "\"appId\": \"com.guideswithme.uk\","
                  "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                  "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                  "\"keys\": [ \"Guernsey\", \"Mercy\" ]"
                "} }";

  GuidesManager::MapT data;
  int version;
  TEST_EQUAL(-1, version = manager.ParseGuidesData("invalidtest", data), ());
  TEST_EQUAL(0, version = manager.ParseGuidesData("{}", data), ());
  manager.RestoreFromParsedData(version, data);
  TEST(!manager.GetGuideInfo("Guernsey", info), ());

  TEST_EQUAL(2, version = manager.ParseGuidesData(str, data), ());
  manager.RestoreFromParsedData(version, data);
  TEST(manager.GetGuideInfo("Guernsey", info), ());
  TEST(info.IsValid(), ());

  TEST_EQUAL(info.GetAdTitle("en"), "UK title", ());
  TEST_EQUAL(info.GetAdMessage("en"), "UK message", ());
  TEST_EQUAL(info.GetAdTitle("ru"), "ВБ заголовок", ());
  TEST_EQUAL(info.GetAdMessage("ru"), "ВБ сообщение", ());
  TEST_EQUAL(info.GetAdTitle("zh"), "UK title", ());
  TEST_EQUAL(info.GetAdMessage("zh"), "UK message", ());

  GuideInfo info1;
  TEST(manager.GetGuideInfo("Mercy", info1), ());
  TEST(info1.IsValid(), ());
  TEST_EQUAL(info, info1, ());

  TEST(!manager.GetGuideInfo("Minsk", info), ());
}

UNIT_TEST(Guides_CorrectlyParseData)
{
  GuidesManager manager;
  GuideInfo info;

  string strLondonIsle =  "{ \"version\": 2,"
                            "\"UK_1\": {"
                            "\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                            "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                            "\"keys\": [ \"London\" ]"
                          "}, \"UK_2\": {"
                            "\"url\": \"https://play.google.com/store/apps/details?id=com.guidewithme.uk\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                            "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                            "\"keys\": [ \"Isle of Man\" ]"
                          "} }";

  string validKeys[] = { "London", "Isle of Man" };
  string invalidKeys[] = { "london", "Lond", "Isle", "Man" };

  GuidesManager::MapT data;
  int version;
  TEST_EQUAL(2, version = manager.ParseGuidesData(strLondonIsle, data), ());
  manager.RestoreFromParsedData(version, data);
  for (size_t i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    TEST(manager.GetGuideInfo(validKeys[i], info), (i));
    TEST(info.IsValid(), ());
  }

  for (size_t i = 0; i < ARRAY_SIZE(invalidKeys); ++i)
    TEST(!manager.GetGuideInfo(invalidKeys[i], info), (i));

  char const * guidesArray[][2] =
  {
    { "https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665", "com.guideswithme.uk" },
    { "https://play.google.com/store/apps/details?id=com.guidewithme.uk", "com.guideswithme.uk" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    TEST(manager.GetGuideInfo(validKeys[i], info), ());
    TEST(info.IsValid(), ());
    TEST_EQUAL(info.GetURL(), guidesArray[i][0], (i));
    TEST_EQUAL(info.GetAppID(), guidesArray[i][1], (i));
  }
}

UNIT_TEST(Guides_ComplexNames)
{
  GuidesManager manager;
  GuideInfo info;

  string const strLondonIsle = "{ \"version\": 123456,"
                            "\"Côte_d'Ivoire\": {"
                            "\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"Côte_d'Ivoire title\", \"ru\": \"КДВар заголовок\" },"
                            "\"adMessages\": { \"en\": \"Côte_d'Ivoire message\", \"ru\": \"КДВар сообщение\" },"
                            "\"keys\": [ \"Côte_d'Ivoire\" ]"
                          "}, \"Беларусь\": {"
                            "\"url\": \"https://play.google.com/store/apps/details?id=com.guidewithme.uk\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"Belarus title\", \"ru\": \"РБ заголовок\" },"
                            "\"adMessages\": { \"en\": \"Belarus message\", \"ru\": \"РБ сообщение\" },"
                            "\"keys\": [ \"Беларусь\" ]"
                          "} }";

  string validKeys[] = { "Côte_d'Ivoire", "Беларусь" };
  string invalidKeys[] = { "Не Беларусь", "Côte_d'IvoireCôte_d'IvoireCôte_d'Ivoire" };

  GuidesManager::MapT data;
  int version;
  TEST_EQUAL(123456, version = manager.ParseGuidesData(strLondonIsle, data), ());
  manager.RestoreFromParsedData(version, data);
  for (size_t i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    TEST(manager.GetGuideInfo(validKeys[i], info), (i));
    TEST(info.IsValid(), ());
  }

  for (size_t i = 0; i < ARRAY_SIZE(invalidKeys); ++i)
    TEST(!manager.GetGuideInfo(invalidKeys[i], info), (i));
}

UNIT_TEST(Guides_SaveRestoreFromFile)
{
  GuidesManager manager;
  GuideInfo info;

  string const strLondonIsle =  "{ \"version\": 2,"
                            "\"UK_1\": {"
                            "\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                            "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                            "\"keys\": [ \"London\" ]"
                          "}, \"UK_2\": {"
                            "\"url\": \"https://play.google.com/store/apps/details?id=com.guidewithme.uk\","
                            "\"appId\": \"com.guideswithme.uk\","
                            "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                            "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                            "\"keys\": [ \"Isle of Man\" ]"
                          "} }";

  GuidesManager::MapT data;
  int version;
  TEST_EQUAL(2, version = manager.ParseGuidesData(strLondonIsle, data), ());
  manager.RestoreFromParsedData(version, data);

  string const path = manager.GetDataFileFullPath();
  {
    FileWriter writer(path);
    writer.Write(strLondonIsle.c_str(), strLondonIsle.size());
  }

  TEST(manager.RestoreFromFile(), ());

  string validKeys[] = { "London", "Isle of Man" };
  char const * guidesArray [][2] =
  {
    { "https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665", "com.guideswithme.uk" },
    { "https://play.google.com/store/apps/details?id=com.guidewithme.uk", "com.guideswithme.uk" }
  };

  for (size_t i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    TEST(manager.GetGuideInfo(validKeys[i], info), (i));
    TEST(info.IsValid(), ());
    TEST_EQUAL(info.GetURL(), guidesArray[i][0], (i));
    TEST_EQUAL(info.GetAppID(), guidesArray[i][1], (i));
  }

  FileWriter::DeleteFileX(path);
}

UNIT_TEST(Guides_CheckDataFiles)
{
  string const path = GetPlatform().WritableDir();
  string arr[] = { "android-guides.json", "ios-guides.json" };

  GuidesManager manager;
  GuideInfo info;

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    FileReader reader(path + arr[i]);
    string str;
    reader.ReadAsString(str);

    GuidesManager::MapT data;
    int version;
    TEST_LESS(0, version = manager.ParseGuidesData(str, data), ());
    manager.RestoreFromParsedData(version, data);
    TEST(manager.GetGuideInfo("UK_England", info), ());
    TEST(info.IsValid(), ());
  }
}

} // namespace guides
