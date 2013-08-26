#include "../../testing/testing.hpp"

#include "../guides.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"


UNIT_TEST(Guides_SmokeTest)
{
  guides::GuidesManager manager;
  guides::GuideInfo info;

  string str =  "{ \"UK\": {"
                  "\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\","
                  "\"appId\": \"com.guideswithme.uk\","
                  "\"adTitles\": { \"en\": \"UK title\", \"ru\": \"ВБ заголовок\" },"
                  "\"adMessages\": { \"en\": \"UK message\", \"ru\": \"ВБ сообщение\" },"
                  "\"keys\": [ \"Guernsey\", \"Mercy\" ]"
                "} }";

  TEST(!manager.ValidateAndParseGuidesData("invalidtest"), ());
  TEST(manager.ValidateAndParseGuidesData("{}"), ());
  TEST(!manager.GetGuideInfo("Guernsey", info), ());

  TEST(manager.ValidateAndParseGuidesData(str), ());
  TEST(manager.GetGuideInfo("Guernsey", info), ());
  TEST(info.IsValid(), ());

  TEST_EQUAL(info.GetAdTitle("en"), "UK title", ());
  TEST_EQUAL(info.GetAdMessage("en"), "UK message", ());
  TEST_EQUAL(info.GetAdTitle("ru"), "ВБ заголовок", ());
  TEST_EQUAL(info.GetAdMessage("ru"), "ВБ сообщение", ());
  TEST_EQUAL(info.GetAdTitle("zh"), "UK title", ());
  TEST_EQUAL(info.GetAdMessage("zh"), "UK message", ());

  guides::GuideInfo info1;
  TEST(manager.GetGuideInfo("Mercy", info1), ());
  TEST(info1.IsValid(), ());
  TEST_EQUAL(info, info1, ());

  TEST(!manager.GetGuideInfo("Minsk", info), ());
}

UNIT_TEST(Guides_CorrectlyParseData)
{
  guides::GuidesManager manager;
  guides::GuideInfo info;

  string strLondonIsle =  "{ \"UK_1\": {"
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

  TEST(manager.ValidateAndParseGuidesData(strLondonIsle), ());
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
  guides::GuidesManager manager;
  guides::GuideInfo info;

  string strLondonIsle = "{\"Côte_d'Ivoire\": {"
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

  TEST(manager.ValidateAndParseGuidesData(strLondonIsle), ());
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
  guides::GuidesManager manager;
  guides::GuideInfo info;

  string strLondonIsle =  "{ \"UK_1\": {"
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

  TEST(manager.ValidateAndParseGuidesData(strLondonIsle), ());

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

  guides::GuidesManager manager;
  guides::GuideInfo info;

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
  {
    FileReader reader(path + arr[i]);
    string str;
    reader.ReadAsString(str);

    TEST(manager.ValidateAndParseGuidesData(str), ());
    TEST(manager.GetGuideInfo("UK_England", info), ());
    TEST(info.IsValid(), ());
  }
}
