#import <XCTest/XCTest.h>
#import "MWMTextToSpeech+CPP.h"

#include "LocaleTranslator.h"

@interface MWMTextToSpeechTest : XCTestCase

@end

@implementation MWMTextToSpeechTest

- (void)testAvailableLanguages
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  std::vector<std::pair<std::string, std::string>> langs = tts.availableLanguages;
  decltype(langs)::value_type const defaultLang = std::make_pair("en-US", "English (United States)");
  XCTAssertTrue(std::find(langs.begin(), langs.end(), defaultLang) != langs.end());
}
// The default language is picked from the tags the system reports for the preferred languages, which
// carry script and region subtags that a plain prefix test cannot tell apart.
- (void)testLanguageCodeOfSystemReportedTags
{
  using locale_translator::bcp47ToTwineLanguage;
  XCTAssertEqual(bcp47ToTwineLanguage(@"en-US"), "en");
  XCTAssertEqual(bcp47ToTwineLanguage(@"pt-BR"), "pt-BR");
  XCTAssertEqual(bcp47ToTwineLanguage(@"pt-PT"), "pt");
  XCTAssertEqual(bcp47ToTwineLanguage(@"es-MX"), "es-MX");

  // Simplified unless the script or the region says otherwise.
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh"), "zh-Hans");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-Hans"), "zh-Hans");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-Hans-CN"), "zh-Hans");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-CN"), "zh-Hans");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-Hant"), "zh-Hant");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-Hant-TW"), "zh-Hant");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-TW"), "zh-Hant");
  XCTAssertEqual(bcp47ToTwineLanguage(@"zh-HK"), "zh-Hant");
  XCTAssertEqual(bcp47ToTwineLanguage(@"yue-HK"), "yue-HK");

  // A three-letter primary subtag is not a twine language, and must not be truncated into one.
  XCTAssertEqual(bcp47ToTwineLanguage(@"fil-PH"), "");
  XCTAssertEqual(bcp47ToTwineLanguage(@"en_US"), "en");
}

- (void)testTranslateLocaleWithTwineString
{
  XCTAssertEqual(tts::translateLocale("en"), "English");
}

- (void)testTranslateLocaleWithBcp47String
{
  XCTAssertEqual(tts::translateLocale("en-US"), "English (United States)");
}

- (void)testTranslateLocaleWithUnknownString
{
  XCTAssertEqual(tts::translateLocale("unknown"), "");
}

@end
