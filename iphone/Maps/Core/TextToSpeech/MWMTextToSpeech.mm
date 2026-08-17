#import <AVFoundation/AVFoundation.h>
#import "MWMRouter.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"
#import "TTSTester.h"

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

MWMTTSLanguage * LanguageFromPair(std::pair<std::string, std::string> const & language);
MWMTTSLanguage * LanguageWithBcp47(NSString * bcp47);

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

AVSpeechSynthesisVoice * bestAvailableVoice(NSString * locale)
{
  AVSpeechSynthesisVoice * bestVoice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  for (AVSpeechSynthesisVoice * voice in [AVSpeechSynthesisVoice speechVoices])
  {
    if (![voice.language isEqualToString:locale])
      continue;
    if (!bestVoice || voice.quality > bestVoice.quality)
      bestVoice = voice;
  }
  return bestVoice;
}

using Observer = id<MWMTextToSpeechObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMTTSLanguage ()

@property(nonatomic) NSString * bcp47;
@property(nonatomic) NSString * title;

@end

@implementation MWMTTSLanguage

+ (instancetype)languageWithBcp47:(NSString *)bcp47 title:(NSString *)title
{
  MWMTTSLanguage * language = [[MWMTTSLanguage alloc] initLanguageWithBcp47:bcp47 title:title];
  return language;
}

- (instancetype)initLanguageWithBcp47:(NSString *)bcp47 title:(NSString *)title
{
  self = [super init];
  if (self)
  {
    _bcp47 = bcp47;
    _title = title;
  }
  return self;
}

- (BOOL)isEqual:(id)object
{
  if (self == object)
    return YES;
  if (![object isKindOfClass:[MWMTTSLanguage class]])
    return NO;
  MWMTTSLanguage * language = object;
  return [self.bcp47 isEqualToString:language.bcp47];
}

- (NSUInteger)hash
{
  return self.bcp47.hash;
}

@end

@interface MWMTextToSpeech () <AVSpeechSynthesizerDelegate>
{
  std::vector<std::pair<std::string, std::string>> _availableLanguages;
}

@property(nonatomic) AVSpeechSynthesizer * speechSynthesizer;
@property(nonatomic) AVSpeechSynthesisVoice * speechVoice;
@property(nonatomic) AVAudioPlayer * audioPlayer;
@property(nonatomic) TTSTester * ttsTester;

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
  tts->_availableLanguages = availableLanguages();
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
    _ttsTester = [[TTSTester alloc] init];

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
+ (NSArray<MWMTTSLanguage *> *)preferredLanguages
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  NSMutableArray<MWMTTSLanguage *> * languages = [NSMutableArray arrayWithCapacity:3];

  MWMTTSLanguage * standard = LanguageFromPair(tts.standardLanguage);
  [languages addObject:standard];

  NSString * currentBcp47 = [AVSpeechSynthesisVoice currentLanguageCode];
  if (currentBcp47.length && ![currentBcp47 isEqualToString:standard.bcp47])
  {
    MWMTTSLanguage * current = LanguageWithBcp47(currentBcp47);
    if (current.title.length == 0 || [[self availableLanguages] containsObject:current])
      [languages addObject:current];
  }

  NSString * savedLanguage = [self savedLanguage];
  if (savedLanguage.length && ![savedLanguage isEqualToString:currentBcp47] &&
      ![savedLanguage isEqualToString:standard.bcp47])
  {
    [languages addObject:LanguageWithBcp47(savedLanguage)];
  }

  return languages;
}
+ (NSArray<MWMTTSLanguage *> *)availableLanguages
{
  std::vector<std::pair<std::string, std::string>> languages = [MWMTextToSpeech tts].availableLanguages;
  NSMutableArray<MWMTTSLanguage *> * result = [NSMutableArray arrayWithCapacity:languages.size()];
  for (auto const & language : languages)
    [result addObject:LanguageFromPair(language)];
  return result;
}
+ (void)setNotificationsLanguage:(MWMTTSLanguage *)language
{
  [[MWMTextToSpeech tts] setNotificationsLocale:language.bcp47];
}
+ (void)playRandomTestString
{
  [[MWMTextToSpeech tts].ttsTester playRandomTestString];
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
    voice = bestAvailableVoice(loc);
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

namespace
{
MWMTTSLanguage * LanguageFromPair(std::pair<std::string, std::string> const & language)
{
  return [MWMTTSLanguage languageWithBcp47:@(language.first.c_str()) title:@(language.second.c_str())];
}

MWMTTSLanguage * LanguageWithBcp47(NSString * bcp47)
{
  std::string const bcp47String = bcp47.UTF8String;
  return [MWMTTSLanguage languageWithBcp47:bcp47 title:@(tts::translateLocale(bcp47String).c_str())];
}
}  // namespace
