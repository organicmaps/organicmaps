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
NSString * const kUserDefaultsTTSVoiceIdentifier = @"UserDefaultsTTSVoiceIdentifier";
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

// Returns the base language part of a BCP-47 tag (e.g., "en" for "en-US"). Returns the input unchanged
// if there is no region suffix.
static NSString * BaseLanguage(NSString * bcp47)
{
  NSRange const dash = [bcp47 rangeOfString:@"-"];
  return dash.location == NSNotFound ? bcp47 : [bcp47 substringToIndex:dash.location];
}

// Among voices sharing a display name, returns YES if `candidate` represents `requestedLanguage`
// better than `existing`: prefer an exact language-tag match, then higher quality.
static BOOL IsBetterVoiceForLanguage(AVSpeechSynthesisVoice * candidate, AVSpeechSynthesisVoice * existing,
                                     NSString * requestedLanguage)
{
  BOOL const candidateExact = [candidate.language isEqualToString:requestedLanguage];
  BOOL const existingExact = [existing.language isEqualToString:requestedLanguage];
  if (candidateExact != existingExact)
    return candidateExact;
  return candidate.quality > existing.quality;
}

NSArray<AVSpeechSynthesisVoice *> * availableVoicesForLanguage(NSString * language)
{
  if (language.length == 0)
    return @[];

  NSArray<AVSpeechSynthesisVoice *> * allVoices = [AVSpeechSynthesisVoice speechVoices];
  if (allVoices.count == 0)
    return @[];

  NSString * baseLanguage = BaseLanguage(language);
  // iOS exposes the same voice under several AVSpeechSynthesisVoice objects: quality tiers / aliases
  // of one voice (same language), and — for the Eloquence set (Eddy, Flo, Grandma, …) — the very same
  // named voice in every language (en-US, en-GB, …). Base-language matching pulls in several of these,
  // so collapse by the display name the user actually sees, keeping the variant that best fits the
  // requested language (exact tag first, then highest quality). Otherwise identical names pile up.
  NSMutableDictionary<NSString *, AVSpeechSynthesisVoice *> * bestByName = [NSMutableDictionary dictionary];
  for (AVSpeechSynthesisVoice * voice in allVoices)
  {
    // Match by base language so e.g. en-GB and en-AU show up for en-US.
    if (![BaseLanguage(voice.language) isEqualToString:baseLanguage])
      continue;
    AVSpeechSynthesisVoice * existing = bestByName[voice.name];
    if (!existing || IsBetterVoiceForLanguage(voice, existing, language))
      bestByName[voice.name] = voice;
  }

  NSMutableArray<AVSpeechSynthesisVoice *> * filteredVoices = [[bestByName allValues] mutableCopy];
  [filteredVoices sortUsingComparator:^NSComparisonResult(AVSpeechSynthesisVoice * v1, AVSpeechSynthesisVoice * v2) {
    return [v1.name compare:v2.name];
  }];
  return [filteredVoices copy];
}

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

  // Drop the saved voice if it's no longer compatible with the chosen language. The voice picker
  // uses base-language matching (see availableVoicesForLanguage), so e.g. an en-AU voice stays
  // valid for en-US but is cleared when switching to fr-FR.
  NSString * savedVoiceIdentifier = [[self class] savedVoiceIdentifier];
  if (savedVoiceIdentifier.length > 0)
  {
    AVSpeechSynthesisVoice * savedVoice = [AVSpeechSynthesisVoice voiceWithIdentifier:savedVoiceIdentifier];
    BOOL const stillCompatible =
        savedVoice && (locale.length == 0 || [BaseLanguage(savedVoice.language) isEqualToString:BaseLanguage(locale)]);
    if (!stillCompatible)
      [ud removeObjectForKey:kUserDefaultsTTSVoiceIdentifier];
  }

  [self createVoice:locale];
}

- (void)setVoiceIdentifier:(NSString *)voiceIdentifier
{
  [[self class] setSavedVoiceIdentifier:voiceIdentifier];
  NSString * locale = [[self class] savedLanguage];
  [self createVoice:locale];
}

- (AVSpeechSynthesisVoice *)currentVoice
{
  // Recreate the voice if it was invalidated (applicationDidBecomeActive nils it to re-enumerate
  // installed voices) so callers such as the settings screen never observe a transient nil after the
  // app returns from the background.
  if (!self.speechVoice)
    [self createVoice:[[self class] savedLanguage]];
  return self.speechVoice;
}

+ (NSString *)displayNameForVoice:(AVSpeechSynthesisVoice *)voice
{
  if (!voice)
    return nil;
  // Default-quality voices show just the name; mark the higher tiers so users can tell them apart.
  // Premium (quality == 3) is iOS 16+, so detect it as "above Enhanced" to avoid referencing the
  // AVSpeechSynthesisVoiceQualityPremium symbol on the iOS 15 deployment target.
  if (voice.quality == AVSpeechSynthesisVoiceQualityEnhanced)
    return
        [NSString stringWithFormat:@"%@ (%@)", voice.name, NSLocalizedString(@"pref_tts_voice_quality_enhanced", @"")];
  if (voice.quality > AVSpeechSynthesisVoiceQualityEnhanced)
    return
        [NSString stringWithFormat:@"%@ (%@)", voice.name, NSLocalizedString(@"pref_tts_voice_quality_premium", @"")];
  return voice.name;
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

+ (NSString *)savedVoiceIdentifier
{
  return [NSUserDefaults.standardUserDefaults stringForKey:kUserDefaultsTTSVoiceIdentifier];
}

+ (void)setSavedVoiceIdentifier:(NSString *)voiceIdentifier
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if (voiceIdentifier.length > 0)
    [ud setObject:voiceIdentifier forKey:kUserDefaultsTTSVoiceIdentifier];
  else
    [ud removeObjectForKey:kUserDefaultsTTSVoiceIdentifier];
  [ud synchronize];
}

+ (NSArray<AVSpeechSynthesisVoice *> *)availableVoicesForLanguage:(NSString *)language
{
  return availableVoicesForLanguage(language);
}

- (void)createVoice:(NSString *)locale
{
  if (!self.speechSynthesizer)
  {
    self.speechSynthesizer = [[AVSpeechSynthesizer alloc] init];
    self.speechSynthesizer.delegate = self;
  }

  // First, try to use saved voice identifier if available. Match by base language so a voice such
  // as en-AU stays valid when the user keeps the en-US locale.
  NSString * savedVoiceIdentifier = [[self class] savedVoiceIdentifier];
  AVSpeechSynthesisVoice * voice = nil;

  if (savedVoiceIdentifier.length > 0)
  {
    voice = [AVSpeechSynthesisVoice voiceWithIdentifier:savedVoiceIdentifier];
    if (voice && locale.length > 0 && ![BaseLanguage(voice.language) isEqualToString:BaseLanguage(locale)])
      voice = nil;
  }

  // If no saved voice or saved voice doesn't match, fall back to language-based selection
  if (!voice)
  {
    NSMutableArray<NSString *> * candidateLocales = [@[kDefaultLanguage, @"en-GB"] mutableCopy];

    if (locale)
      [candidateLocales insertObject:locale atIndex:0];
    else
      LOG(LWARNING, ("locale is nil. Trying default locale."));

    for (NSString * loc in candidateLocales)
    {
      voice = bestAvailableVoice(loc);
      if (voice)
        break;
    }
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

  // Interrupt any in-flight test utterance so a fresh tap on Test Voice Directions speaks the
  // currently selected voice immediately instead of queueing behind the previous one.
  if (self.speechSynthesizer.isSpeaking)
    [self.speechSynthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];

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
