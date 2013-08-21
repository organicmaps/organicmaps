#include "../../testing/testing.hpp"

#include "../../map/guides.hpp"

UNIT_TEST(ValidTest)
{
  guides::GuidesManager manager;
  string str = "{\"Guernsey\":{\"name\": \"UK Travel Guide with Me\",\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\",\"appId\": \"com.guideswithme.uk\"}}";

  TEST(!manager.ValidateAndParseGuidesData("invalidtest"), (""));
  TEST(manager.ValidateAndParseGuidesData("{}"), (""));

  TEST(manager.ValidateAndParseGuidesData(str),("Has 'Guernsey'"));
}

UNIT_TEST(ParseDataTest)
{
  guides::GuidesManager manager;
  string str = "{\"Guernsey\":{\"name\": \"UK Travel Guide with Me\",\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\",\"appId\": \"com.guideswithme.uk\"}}";
  string key = "Guernsey";
  guides::GuideInfo info;

  manager.ValidateAndParseGuidesData("{}");
  TEST(!manager.GetGuideInfo(key, info), ("Empty data set"));

  manager.ValidateAndParseGuidesData(str);
  TEST(manager.GetGuideInfo(key, info), ("Has info for Guernsey"));
  TEST(!manager.GetGuideInfo("Minsk", info), ("Has not info for Minsk"));

  string strLondonIsle = "{\"London\": {\"name\": \"UK Travel Guide with Me\",\"url\": \"https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665\",\"appId\": \"com.guideswithme.uk\"},\"Isle of Man\": {\"name\": \"UK Travel Guide with Me\",\"url\": \"https://play.google.com/store/apps/details?id=com.guidewithme.uk\",\"appId\": \"com.guideswithme.uk\"}}";
  string validKeys[] = {"London", "Isle of Man"};
  string invalidKeys[] = {"Minsk", "Blabla"};

  TEST(manager.ValidateAndParseGuidesData(strLondonIsle), ("MUST BE PARSED"));
  for (int i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    string key = validKeys[i];
    TEST(manager.GetGuideInfo(key, info), ("Has info for:", key));
  }

  for (int i = 0; i < ARRAY_SIZE(invalidKeys); ++i)
  {
    string key = invalidKeys[i];
    TEST(!manager.GetGuideInfo(key, info), ("Has no info for:", key));
  }

  guides::GuideInfo guidesArray [] =
  {
    guides::GuideInfo("UK Travel Guide with Me", "https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665", "com.guideswithme.uk"),
    guides::GuideInfo("UK Travel Guide with Me", "https://play.google.com/store/apps/details?id=com.guidewithme.uk", "com.guideswithme.uk")
  };
  for (int i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    string key = validKeys[i];
    manager.GetGuideInfo(key, info);
    TEST((info == guidesArray[i]), (i));
  }
}

UNIT_TEST(RestoreFromFile)
{
  // @todo put or create file with content
  guides::GuidesManager manager;
  guides::GuideInfo info;
  manager.RestoreFromFile();

  string validKeys[] = {"London", "Isle of Man"};
  guides::GuideInfo guidesArray [] =
  {
    guides::GuideInfo("UK Travel Guide with Me", "https://itunes.apple.com/app/uk-travel-guide-with-me/id687855665", "com.guideswithme.uk"),
    guides::GuideInfo("UK Travel Guide with Me", "https://play.google.com/store/apps/details?id=com.guidewithme.uk", "com.guideswithme.uk")
  };
  for (int i = 0; i < ARRAY_SIZE(validKeys); ++i)
  {
    string key = validKeys[i];
    manager.GetGuideInfo(key, info);
    TEST((info == guidesArray[i]), (i));
  }
}
