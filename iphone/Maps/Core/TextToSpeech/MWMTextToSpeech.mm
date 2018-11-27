#import <AVFoundation/AVFoundation.h>
#import <Crashlytics/Crashlytics.h>
#import "MWMCommon.h"
#import "MWMRouter.h"
#import "MWMTextToSpeech+CPP.h"
#import "Statistics.h"

#include "LocaleTranslator.h"

#include "Framework.h"

#include "platform/languages.hpp"

#include "base/assert.hpp"
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
@property(nonatomic) AVAudioPlayer * audioPlayer;

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

    NSError * err = nil;
    if (![[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback
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

- (void)dealloc
{
  [[AVAudioSession sharedInstance] setActive:NO
                                 withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                       error:nil];
  self.speechSynthesizer.delegate = nil;
}
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
      voice = [AVSpeechSynthesisVoice voiceWithIdentifier:AVSpeechSynthesisVoiceIdentifierAlex];
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
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = AVSpeechUtteranceDefaultSpeechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)playTurnNotifications
{
  auto stopSession = ^{
    if (self.speechSynthesizer.isSpeaking)
      return;
    [[AVAudioSession sharedInstance]
          setActive:NO
        withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
              error:nil];
  };

  if (![MWMRouter isOnRoute] || !self.active)
  {
    stopSession();
    return;
  }

  if (![self isValid])
    [self createVoice:[[self class] savedLanguage]];

  if (![self isValid])
  {
    stopSession();
    return;
  }

  NSArray<NSString *> * turnNotifications = [MWMRouter turnNotifications];
  if (turnNotifications.count == 0)
  {
    stopSession();
    return;
  }
  else
  {
    if (![[AVAudioSession sharedInstance] setActive:YES error:nil])
      return;

    for (NSString * notification in turnNotifications)
      [self speakOneString:notification];
  }
}

- (void)playWarningSound
{
  if (!GetFramework().GetRoutingManager().GetSpeedCamManager().ShouldPlayBeepSignal())
    return;

  [self.audioPlayer play];
}

- (AVAudioPlayer *)audioPlayer
{
  if (!_audioPlayer)
  {
    if (auto url = [[NSBundle mainBundle] URLForResource:@"Alert 5" withExtension:@"m4a"])
    {
      NSError * error = nil;
      _audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
      CHECK(!error, (error.localizedDescription.UTF8String));
    }
    else
    {
      CHECK(false, ("Speed warning file not found"));
    }
  }

  return _audioPlayer;
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
