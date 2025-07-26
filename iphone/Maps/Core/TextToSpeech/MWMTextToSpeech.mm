#import <AVFoundation/AVFoundation.h>
#import "MWMRouter.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

#include "LocaleTranslator.h"

#include <CoreApi/Framework.h>

#include "platform/languages.hpp"

using namespace locale_translator;

namespace
{
NSString * const kUserDefaultsTTSLanguageBcp47 = @"UserDefaultsTTSLanguageBcp47";
NSString * const kIsTTSEnabled = @"UserDefaultsNeedToEnableTTS";
NSString * const kIsStreetNamesTTSEnabled = @"UserDefaultsNeedToEnableStreetNamesTTS";
NSString * const kDefaultLanguage = @"en-US";

std::vector<std::pair<std::string, std::string>> availableLanguages()
{
  NSArray<AVSpeechSynthesisVoice *> * voices = [AVSpeechSynthesisVoice speechVoices];
  std::vector<std::pair<std::string, std::string>> native;
  for (AVSpeechSynthesisVoice * v in voices)
    native.emplace_back(make_pair(bcp47ToTwineLanguage(v.language), v.language.UTF8String));

  using namespace routing::turns::sound;
  std::vector<std::pair<std::string, std::string>> result;
  for (auto const & [twineRouting, _] : kLanguageList)
  {
    for (auto const & [twineVoice, bcp47Voice] : native)
    {
      if (twineVoice == twineRouting)
      {
        auto pair = std::make_pair(bcp47Voice, tts::translateLocale(bcp47Voice));
        if (std::find(result.begin(), result.end(), pair) == result.end())
          result.emplace_back(std::move(pair));
      }
    }
  }
  return result;
}

using Observer = id<MWMTextToSpeechObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMTextToSpeech () <AVSpeechSynthesizerDelegate>
{
  std::vector<std::pair<std::string, std::string>> _availableLanguages;
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
  dispatch_once(&onceToken, ^{ tts = [[self alloc] initTTS]; });
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

    std::pair<std::string, std::string> const lan =
        std::make_pair(preferedLanguageBcp47.UTF8String, tts::translateLocale(preferedLanguageBcp47.UTF8String));

    if (find(_availableLanguages.begin(), _availableLanguages.end(), lan) != _availableLanguages.end())
      [self setNotificationsLocale:preferedLanguageBcp47];
    else
      [self setNotificationsLocale:kDefaultLanguage];

    NSError * err = nil;
    if (![[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback
                                                 mode:AVAudioSessionModeVoicePrompt
                                              options:AVAudioSessionCategoryOptionInterruptSpokenAudioAndMixWithOthers |
                                                      AVAudioSessionCategoryOptionDuckOthers
                                                error:&err])
    {
      LOG(LWARNING, ("Couldn't configure audio session: ", [err localizedDescription]));
    }

    // Set initial StreetNamesTTS setting
    NSDictionary * dictionary = @{kIsStreetNamesTTSEnabled: @NO};
    [NSUserDefaults.standardUserDefaults registerDefaults:dictionary];

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
- (std::vector<std::pair<std::string, std::string>>)availableLanguages
{
  return _availableLanguages;
}
- (std::pair<std::string, std::string>)standardLanguage
{
  return std::make_pair(kDefaultLanguage.UTF8String, tts::translateLocale(kDefaultLanguage.UTF8String));
}
- (void)setNotificationsLocale:(NSString *)locale
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setObject:locale forKey:kUserDefaultsTTSLanguageBcp47];
  [self createVoice:locale];
}

- (BOOL)isValid
{
  return _speechSynthesizer != nil && _speechVoice != nil;
}
+ (BOOL)isTTSEnabled
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kIsTTSEnabled];
}
+ (void)setTTSEnabled:(BOOL)enabled
{
  if ([self isTTSEnabled] == enabled)
    return;
  auto tts = [self tts];
  if (!enabled)
    [tts setActive:NO];
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:enabled forKey:kIsTTSEnabled];

  [tts onTTSStatusUpdated];
  if (enabled)
    [tts setActive:YES];
}
+ (BOOL)isStreetNamesTTSEnabled
{
  return [NSUserDefaults.standardUserDefaults boolForKey:kIsStreetNamesTTSEnabled];
}
+ (void)setStreetNamesTTSEnabled:(BOOL)enabled
{
  if ([self isStreetNamesTTSEnabled] == enabled)
    return;
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:enabled forKey:kIsStreetNamesTTSEnabled];
  [ud synchronize];
}

- (void)setActive:(BOOL)active
{
  if (![[self class] isTTSEnabled] || self.active == active)
    return;
  if (active && ![self isValid])
    [self createVoice:[[self class] savedLanguage]];
  [MWMRouter enableTurnNotifications:active];
  dispatch_async(dispatch_get_main_queue(), ^{ [self onTTSStatusUpdated]; });
}

- (BOOL)active
{
  return [[self class] isTTSEnabled] && [MWMRouter areTurnNotificationsEnabled];
}
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

  NSMutableArray<NSString *> * candidateLocales = [@[kDefaultLanguage, @"en-GB"] mutableCopy];

  if (locale)
    [candidateLocales insertObject:locale atIndex:0];
  else
    LOG(LWARNING, ("locale is nil. Trying default locale."));

  AVSpeechSynthesisVoice * voice = nil;
  for (NSString * loc in candidateLocales)
  {
    voice = [AVSpeechSynthesisVoice voiceWithLanguage:loc];
    if (voice)
      break;
  }

  self.speechVoice = voice;
  if (voice)
  {
    std::string const twineLang = bcp47ToTwineLanguage(voice.language);
    if (twineLang.empty())
      LOG(LERROR, ("Cannot convert UI locale or default locale to twine language. MWMTextToSpeech "
                   "is invalid."));
    else
      [MWMRouter setTurnNotificationsLocale:@(twineLang.c_str())];
  }
  else
  {
    LOG(LWARNING, ("The UI language and English are not available for TTS. MWMTextToSpeech is invalid."));
  }
}

- (void)speakOneString:(NSString *)textToSpeak
{
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = AVSpeechUtteranceDefaultSpeechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)playTurnNotifications:(NSArray<NSString *> *)turnNotifications
{
  auto stopSession = ^{
    if (self.speechSynthesizer.isSpeaking)
      return;
    [[AVAudioSession sharedInstance] setActive:NO
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

  if (turnNotifications.count == 0)
  {
    stopSession();
    return;
  }
  else
  {
    NSError * err = nil;
    if (![[AVAudioSession sharedInstance] setActive:YES error:&err])
    {
      LOG(LWARNING, ("Couldn't activate audio session: ", [err localizedDescription]));
      return;
    }

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

- (void)play:(NSString *)text
{
  if (![self isValid])
    [self createVoice:[[self class] savedLanguage]];

  [self speakOneString:text];
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
std::string translateLocale(std::string const & localeString)
{
  NSString * nsLocaleString = [NSString stringWithUTF8String:localeString.c_str()];
  NSLocale * locale = [[NSLocale alloc] initWithLocaleIdentifier:nsLocaleString];
  NSString * localizedName = [locale localizedStringForLocaleIdentifier:nsLocaleString];
  localizedName = [localizedName capitalizedString];
  return std::string(localizedName.UTF8String);
}
}  // namespace tts
