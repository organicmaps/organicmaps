#import <AVFoundation/AVFoundation.h>
#import <Crashlytics/Crashlytics.h>
#import "MWMCommon.h"
#import "MWMRouter.h"
#import "MWMTextToSpeech+CPP.h"
#import "Statistics.h"

#include "LocaleTranslator.h"

#include "platform/languages.hpp"

#include "base/logging.hpp"

using namespace locale_translator;

namespace
{
NSString * const kUserDefaultsTTSLanguageBcp47 = @"UserDefaultsTTSLanguageBcp47";
NSString * const kIsTTSEnabled = @"UserDefaultsNeedToEnableTTS";
NSString * const kDefaultLanguage = @"en-US";

std::vector<std::pair<string, string>> availableLanguages()
{
  NSArray<AVSpeechSynthesisVoice *> * voices = [AVSpeechSynthesisVoice speechVoices];
  std::vector<std::pair<string, string>> native;
  for (AVSpeechSynthesisVoice * v in voices)
    native.emplace_back(make_pair(bcp47ToTwineLanguage(v.language), v.language.UTF8String));

  using namespace routing::turns::sound;
  std::vector<std::pair<string, string>> result;
  for (auto const & p : kLanguageList)
  {
    for (std::pair<string, string> const & lang : native)
    {
      if (lang.first == p.first)
      {
        // Twine names are equal. Make a pair: bcp47 name, localized name.
        result.emplace_back(make_pair(lang.second, p.second));
        break;
      }
    }
  }
  return result;
}

using Observer = id<MWMTextToSpeechObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMTextToSpeech ()<AVSpeechSynthesizerDelegate>
{
  std::vector<std::pair<string, string>> _availableLanguages;
}

@property(nonatomic) AVSpeechSynthesizer * speechSynthesizer;
@property(nonatomic) AVSpeechSynthesisVoice * speechVoice;
@property(nonatomic) float speechRate;
@property(nonatomic) AVAudioSession * audioSession;

@property(nonatomic) Observers * observers;

@end

@implementation MWMTextToSpeech

+ (MWMTextToSpeech *)tts
{
  static dispatch_once_t onceToken;
  static MWMTextToSpeech * tts = nil;
  dispatch_once(&onceToken, ^{
    tts = [[super alloc] initTTS];
  });
  return tts;
}

+ (void)applicationDidBecomeActive
{
  auto tts = [self tts];
  tts.speechSynthesizer = nil;
  tts.speechVoice = nil;
}

- (instancetype)initTTS
{
  self = [super init];
  if (self)
  {
    _availableLanguages = availableLanguages();
    _observers = [Observers weakObjectsHashTable];

    NSString * saved = [[self class] savedLanguage];
    NSString * preferedLanguageBcp47;
    if (saved.length)
      preferedLanguageBcp47 = saved;
    else
      preferedLanguageBcp47 = [AVSpeechSynthesisVoice currentLanguageCode];

    std::pair<string, string> const lan =
        make_pair(preferedLanguageBcp47.UTF8String,
                  tts::translatedTwine(bcp47ToTwineLanguage(preferedLanguageBcp47)));

    if (find(_availableLanguages.begin(), _availableLanguages.end(), lan) !=
        _availableLanguages.end())
      [self setNotificationsLocale:preferedLanguageBcp47];
    else
      [self setNotificationsLocale:kDefaultLanguage];

    // Before 9.0 version iOS has an issue with speechRate. AVSpeechUtteranceDefaultSpeechRate does
    // not work correctly.
    // It's a work around for iOS 7.x and 8.x.
    _speechRate =
        isIOSVersionLessThan(@"7.1.1")
            ? 0.3
            : (isIOSVersionLessThan(@"9.0.0") ? 0.15 : AVSpeechUtteranceDefaultSpeechRate);

    NSError * err = nil;
    _audioSession = [AVAudioSession sharedInstance];
    if (![_audioSession setCategory:AVAudioSessionCategoryPlayback
                        withOptions:AVAudioSessionCategoryOptionMixWithOthers |
                                    AVAudioSessionCategoryOptionDuckOthers
                              error:&err])
    {
      LOG(LWARNING, ("[ setCategory]] error.", [err localizedDescription]));
    }
    self.active = YES;
  }
  return self;
}

- (void)dealloc { self.speechSynthesizer.delegate = nil; }
- (std::vector<std::pair<string, string>>)availableLanguages { return _availableLanguages; }
- (void)setNotificationsLocale:(NSString *)locale
{
  [Statistics logEvent:kStatEventName(kStatTTSSettings, kStatChangeLanguage)
        withParameters:@{kStatValue : locale}];
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:locale forKey:kUserDefaultsTTSLanguageBcp47];
  [ud synchronize];
  [self createVoice:locale];
}

- (BOOL)isValid { return _speechSynthesizer != nil && _speechVoice != nil; }
+ (BOOL)isTTSEnabled { return [NSUserDefaults.standardUserDefaults boolForKey:kIsTTSEnabled]; }
+ (void)setTTSEnabled:(BOOL)enabled
{
  if ([self isTTSEnabled] == enabled)
    return;
  auto tts = [self tts];
  if (!enabled)
    [tts setActive:NO];
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:enabled forKey:kIsTTSEnabled];
  [ud synchronize];

  [tts onTTSStatusUpdated];
  if (enabled)
    [tts setActive:YES];
}

- (void)setActive:(BOOL)active
{
  if (![[self class] isTTSEnabled] || self.active == active)
    return;
  if (active && ![self isValid])
    [self createVoice:[[self class] savedLanguage]];
  [self setAudioSessionActive:active];
  [MWMRouter enableTurnNotifications:active];
  dispatch_async(dispatch_get_main_queue(), ^{
    [self onTTSStatusUpdated];
  });
}

- (BOOL)active { return [[self class] isTTSEnabled] && [MWMRouter areTurnNotificationsEnabled]; }
+ (NSString *)savedLanguage
{
  return [NSUserDefaults.standardUserDefaults stringForKey:kUserDefaultsTTSLanguageBcp47];
}

- (void)createVoice:(NSString *)locale
{
  if (!self.speechSynthesizer)
  {
    self.speechSynthesizer = [[AVSpeechSynthesizer alloc] init];
    self.speechSynthesizer.delegate = self;
  }

  NSMutableArray<NSString *> * candidateLocales = [@[ kDefaultLanguage, @"en-GB" ] mutableCopy];

  if (locale)
    [candidateLocales insertObject:locale atIndex:0];
  else
    LOG(LWARNING, ("locale is nil. Trying default locale."));

  NSArray<AVSpeechSynthesisVoice *> * availTTSVoices = [AVSpeechSynthesisVoice speechVoices];
  AVSpeechSynthesisVoice * voice = nil;
  for (NSString * loc in candidateLocales)
  {
    if ([loc isEqualToString:@"en-US"])
      voice = [AVSpeechSynthesisVoice voiceWithLanguage:AVSpeechSynthesisVoiceIdentifierAlex];
    if (voice)
      break;
    for (AVSpeechSynthesisVoice * ttsVoice in availTTSVoices)
    {
      if ([ttsVoice.language isEqualToString:loc])
      {
        voice = [AVSpeechSynthesisVoice voiceWithLanguage:loc];
        break;
      }
    }
    if (voice)
      break;
  }

  self.speechVoice = voice;
  if (voice)
  {
    string const twineLang = bcp47ToTwineLanguage(voice.language);
    if (twineLang.empty())
      LOG(LERROR, ("Cannot convert UI locale or default locale to twine language. MWMTextToSpeech "
                   "is invalid."));
    else
      [MWMRouter setTurnNotificationsLocale:@(twineLang.c_str())];
  }
  else
  {
    LOG(LERROR,
        ("The UI language and English are not available for TTS. MWMTextToSpeech is invalid."));
  }
}

- (void)speakOneString:(NSString *)textToSpeak
{
  CLS_LOG(@"Speak text: %@", textToSpeak);
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = self.speechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)playTurnNotifications
{
  if (![MWMRouter isOnRoute])
    return;

  if (self.active && ![self isValid])
    [self createVoice:[[self class] savedLanguage]];

  if (!self.active || ![self isValid])
    return;

  NSArray<NSString *> * turnNotifications = [MWMRouter turnNotifications];
  for (NSString * notification in turnNotifications)
    [self speakOneString:notification];
}

- (BOOL)setAudioSessionActive:(BOOL)audioSessionActive
{
  NSError * err;
  if (![self.audioSession setActive:audioSessionActive
                        withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                              error:&err])
  {
    LOG(LWARNING,
        ("[[AVAudioSession sharedInstance] setActive]] error.", [err localizedDescription]));
    return NO;
  }
  return YES;
}

#pragma mark - MWMNavigationDashboardObserver

- (void)onTTSStatusUpdated
{
  for (Observer observer in self.observers)
    [observer onTTSStatusUpdated];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMTextToSpeechObserver>)observer
{
  [[self tts].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMTextToSpeechObserver>)observer
{
  [[self tts].observers removeObject:observer];
}

@end

namespace tts
{
string translatedTwine(string const & twine)
{
  auto const & list = routing::turns::sound::kLanguageList;
  auto const it =
      find_if(list.begin(), list.end(),
              [&twine](std::pair<string, string> const & pair) { return pair.first == twine; });

  if (it != list.end())
    return it->second;
  else
    return "";
}
}  // namespace tts
