#import <XCTest/XCTest.h>
#import "MWMTextToSpeech+CPP.h"

#include "LocaleTranslator.h"

@interface MWMTextToSpeechTest : XCTestCase

@end

@implementation MWMTextToSpeechTest

- (void)testAvailableLanguagesAreSpeakable
{
  NSArray<MWMTTSLanguage *> * languages = [MWMTextToSpeech availableLanguages];
  XCTAssertGreaterThan(languages.count, 0);
  for (MWMTTSLanguage * language in languages)
  {
    XCTAssertGreaterThan(language.title.length, 0, @"%@ has no name", language.code);
    XCTAssertGreaterThan([MWMTextToSpeech voicesForLanguage:language].count, 0, @"%@ has no voice", language.code);
    XCTAssertNotNil([MWMTextToSpeech bestVoiceForLanguage:language], @"%@ has no best voice", language.code);
  }
}

// The pickers group voices by the language the notification texts are keyed by. If two languages
// shared a voice, picking it under one of them would silently switch the notifications to the other.
- (void)testEachVoiceBelongsToASingleLanguage
{
  NSMutableDictionary<NSString *, NSString *> * languageByVoice = [NSMutableDictionary dictionary];
  for (MWMTTSLanguage * language in [MWMTextToSpeech availableLanguages])
  {
    for (MWMTTSVoice * voice in [MWMTextToSpeech voicesForLanguage:language])
    {
      NSString * other = languageByVoice[voice.identifier];
      XCTAssertNil(other, @"%@ is listed under both %@ and %@", voice.title, other, language.code);
      languageByVoice[voice.identifier] = language.code;
    }
  }
}

// Regional variants of one voice - the Eloquence set ships Eddy, Flo and others in every region of a
// language - must collapse into a single row.
- (void)testVoiceNamesAreUniqueWithinALanguage
{
  for (MWMTTSLanguage * language in [MWMTextToSpeech availableLanguages])
  {
    NSMutableSet<NSString *> * titles = [NSMutableSet set];
    for (MWMTTSVoice * voice in [MWMTextToSpeech voicesForLanguage:language])
    {
      XCTAssertFalse([titles containsObject:voice.title], @"%@ is listed twice under %@", voice.title, language.code);
      [titles addObject:voice.title];
    }
  }
}

// These pairs share a primary subtag but not a set of notification texts, so they must never be
// offered as alternatives for one another.
- (void)testScriptAndRegionVariantsAreDistinctLanguages
{
  NSArray<NSArray<NSString *> *> * const distinctPairs =
      @[@[@"zh-CN", @"zh-TW"], @[@"pt-PT", @"pt-BR"], @[@"es-ES", @"es-MX"]];
  for (NSArray<NSString *> * pair in distinctPairs)
  {
    std::string const first = locale_translator::bcp47ToTwineLanguage(pair[0]);
    std::string const second = locale_translator::bcp47ToTwineLanguage(pair[1]);
    XCTAssertNotEqual(first, second, @"%@ and %@ must not share a language", pair[0], pair[1]);
  }
}

- (void)testCurrentLanguageIsAvailableAndMatchesCurrentVoice
{
  MWMTTSLanguage * language = [MWMTextToSpeech currentLanguage];
  XCTAssertNotNil(language);
  XCTAssertTrue([[MWMTextToSpeech availableLanguages] containsObject:language]);

  // The settings screen shows these two side by side, so they have to describe the same voice.
  MWMTTSVoice * voice = [MWMTextToSpeech currentVoice];
  XCTAssertNotNil(voice);
  XCTAssertTrue([[MWMTextToSpeech voicesForLanguage:language] containsObject:voice], @"%@ is not offered for %@",
                voice.title, language.code);
}

- (void)testSelectingALanguageKeepsItsBestVoice
{
  MWMTTSLanguage * const initial = [MWMTextToSpeech currentLanguage];
  for (MWMTTSLanguage * language in [MWMTextToSpeech availableLanguages])
  {
    [MWMTextToSpeech setLanguage:language];
    XCTAssertEqualObjects([MWMTextToSpeech currentLanguage], language);
    XCTAssertEqualObjects([MWMTextToSpeech currentVoice], [MWMTextToSpeech bestVoiceForLanguage:language]);
  }
  [MWMTextToSpeech setLanguage:initial];
}

- (void)testSelectingAVoiceSelectsItsLanguage
{
  MWMTTSLanguage * const initial = [MWMTextToSpeech currentLanguage];
  for (MWMTTSLanguage * language in [MWMTextToSpeech availableLanguages])
  {
    for (MWMTTSVoice * voice in [MWMTextToSpeech voicesForLanguage:language])
    {
      [MWMTextToSpeech setVoice:voice];
      XCTAssertEqualObjects([MWMTextToSpeech currentVoice], voice);
      XCTAssertEqualObjects([MWMTextToSpeech currentLanguage], language, @"%@ changed the language", voice.title);
    }
  }
  [MWMTextToSpeech setLanguage:initial];
}

// A hand-picked voice stays in use even once a better one is installed, so anything naming the voice
// of the selected language has to read currentVoice and not bestVoiceForLanguage.
- (void)testSavedVoiceOutranksTheBestInstalledOne
{
  MWMTTSLanguage * const initial = [MWMTextToSpeech currentLanguage];
  for (MWMTTSLanguage * language in [MWMTextToSpeech availableLanguages])
  {
    NSArray<MWMTTSVoice *> * voices = [MWMTextToSpeech voicesForLanguage:language];
    MWMTTSVoice * best = [MWMTextToSpeech bestVoiceForLanguage:language];
    MWMTTSVoice * other = nil;
    for (MWMTTSVoice * voice in voices)
      if (![voice isEqual:best])
        other = voice;
    if (!other)
      continue;

    [MWMTextToSpeech setVoice:other];
    XCTAssertEqualObjects([MWMTextToSpeech currentVoice], other, @"%@ did not stay selected", other.title);
    XCTAssertEqualObjects([MWMTextToSpeech currentLanguage], language);
  }
  [MWMTextToSpeech setLanguage:initial];
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

// iOS 15 has no localized name for an unknown identifier and returns nil rather than an empty string.
- (void)testTranslateLocaleWithUnknownString
{
  XCTAssertEqual(tts::translateLocale("unknown"), "");
}

@end
