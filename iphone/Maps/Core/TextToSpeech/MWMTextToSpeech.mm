#import <AVFoundation/AVFoundation.h>
#import "MWMRouter.h"
#import "MWMTextToSpeech+CPP.h"
#import "SwiftBridge.h"

#include "LocaleTranslator.h"

#include <CoreApi/Framework.h>

#include "platform/languages.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace locale_translator;

NSNotificationName const MWMTextToSpeechVoicesDidChangeNotification = @"MWMTextToSpeechVoicesDidChange";

namespace
{
NSString * const kUserDefaultsTTSLanguageBcp47 = @"UserDefaultsTTSLanguageBcp47";
NSString * const kUserDefaultsTTSVoiceIdentifier = @"UserDefaultsTTSVoiceIdentifier";
NSString * const kIsTTSEnabled = @"UserDefaultsNeedToEnableTTS";
NSString * const kIsStreetNamesTTSEnabled = @"UserDefaultsNeedToEnableStreetNamesTTS";
NSString * const kDefaultLanguage = @"en-US";

// Internal code the notification texts are keyed by, e.g. "en" for en-GB and "zh-Hant" for zh-TW.
// This is the unit the pickers group by: every voice in one group makes the routing engine speak the
// same language, so picking among them cannot change the wording. Note that it deliberately keeps
// pt/pt-BR, es/es-MX and zh-Hans/zh-Hant apart, which a plain "text before the dash" split does not.
std::string TwineLanguage(NSString * bcp47)
{
  return bcp47.length > 0 ? bcp47ToTwineLanguage(bcp47) : std::string();
}

MWMTTSLanguage * LanguageWithCode(std::string const & code)
{
  for (auto const & [listedCode, title] : routing::turns::sound::kLanguageList)
    if (listedCode == code)
      return [MWMTTSLanguage languageWithCode:@(code.c_str()) title:@(std::string(title).c_str())];
  ASSERT(false, ("A voice language outside kLanguageList", code));
  return nil;
}

// Installed voices bucketed by the language they read out. Deriving that language allocates and the
// pickers ask about every language in turn, so group them once and answer from the buckets.
NSDictionary<NSString *, NSArray<AVSpeechSynthesisVoice *> *> * VoicesByLanguage(
    NSArray<AVSpeechSynthesisVoice *> * voices)
{
  NSMutableDictionary<NSString *, NSMutableArray<AVSpeechSynthesisVoice *> *> * buckets =
      [NSMutableDictionary dictionary];
  for (AVSpeechSynthesisVoice * voice in voices)
  {
    std::string const language = TwineLanguage(voice.language);
    if (language.empty())
      continue;
    NSString * key = @(language.c_str());
    NSMutableArray<AVSpeechSynthesisVoice *> * bucket = buckets[key];
    if (!bucket)
    {
      bucket = [NSMutableArray array];
      buckets[key] = bucket;
    }
    [bucket addObject:voice];
  }
  return buckets;
}

// Whether `candidate` represents `preferredTag` better than `existing`: an exact regional match
// first, then a higher quality, then the tag itself so that the choice is stable.
BOOL IsBetterVoice(AVSpeechSynthesisVoice * candidate, AVSpeechSynthesisVoice * existing, NSString * preferredTag)
{
  BOOL const candidateExact = [candidate.language isEqualToString:preferredTag];
  BOOL const existingExact = [existing.language isEqualToString:preferredTag];
  if (candidateExact != existingExact)
    return candidateExact;
  if (candidate.quality != existing.quality)
    return candidate.quality > existing.quality;
  return [candidate.language compare:existing.language] == NSOrderedAscending;
}

// Highest-quality installed voice reading out `language`, preferring the region of `preferredTag`.
AVSpeechSynthesisVoice * BestVoiceForLanguage(NSArray<AVSpeechSynthesisVoice *> * languageVoices,
                                              NSString * preferredTag)
{
  AVSpeechSynthesisVoice * bestVoice = nil;
  for (AVSpeechSynthesisVoice * voice in languageVoices)
    if (!bestVoice || IsBetterVoice(voice, bestVoice, preferredTag))
      bestVoice = voice;
  return bestVoice;
}

// Voices reading out `language`, one per voice name. iOS exposes the same voice under several
// AVSpeechSynthesisVoice objects: the quality tiers of one voice, and - for the Eloquence set (Eddy,
// Flo, Grandma, ...) - the same named voice in every region of the language. Keep the variant closest
// to the device language so that the accent matches what the user hears elsewhere in the system.
NSArray<AVSpeechSynthesisVoice *> * VoicesForLanguage(NSArray<AVSpeechSynthesisVoice *> * languageVoices,
                                                      NSString * preferredTag)
{
  NSMutableDictionary<NSString *, AVSpeechSynthesisVoice *> * bestByName = [NSMutableDictionary dictionary];
  for (AVSpeechSynthesisVoice * voice in languageVoices)
  {
    AVSpeechSynthesisVoice * existing = bestByName[voice.name];
    if (!existing || IsBetterVoice(voice, existing, preferredTag))
      bestByName[voice.name] = voice;
  }

  return [[bestByName allValues]
      sortedArrayUsingComparator:^NSComparisonResult(AVSpeechSynthesisVoice * lhs, AVSpeechSynthesisVoice * rhs) {
        return [lhs.name compare:rhs.name];
      }];
}

// Default-quality voices show just the name; mark the higher tiers so users can tell them apart.
// Premium (quality == 3) is iOS 16+, so detect it as "above Enhanced" to avoid referencing the
// AVSpeechSynthesisVoiceQualityPremium symbol on the iOS 15 deployment target.
NSString * DisplayNameForVoice(AVSpeechSynthesisVoice * voice)
{
  if (voice.quality == AVSpeechSynthesisVoiceQualityEnhanced)
    return [NSString stringWithFormat:@"%@ (%@)", voice.name, L(@"pref_tts_voice_quality_enhanced")];
  if (voice.quality > AVSpeechSynthesisVoiceQualityEnhanced)
    return [NSString stringWithFormat:@"%@ (%@)", voice.name, L(@"pref_tts_voice_quality_premium")];
  return voice.name;
}

// Localized name of the region in a voice's language tag, e.g. "United Kingdom" for en-GB. Only a
// two-letter region subtag is resolved: a script such as the "Hans" of zh-Hans is not a region, and
// the numeric UN M.49 areas some tags use have no country name to show.
NSString * RegionNameForVoice(AVSpeechSynthesisVoice * voice)
{
  NSString * lastSubtag = [voice.language componentsSeparatedByString:@"-"].lastObject;
  if (lastSubtag.length != 2 || [lastSubtag isEqualToString:voice.language])
    return nil;
  return [NSLocale.currentLocale localizedStringForCountryCode:lastSubtag.uppercaseString];
}

// Novelty voices are the only set the API names, and not before iOS 17. Eloquence is recognised by
// the stable prefix of its identifiers. Siri voices are not offered to other apps at all, so there is
// nothing to recognise. An unrecognised voice counts as standard.
MWMTTSVoiceGroup GroupForVoice(AVSpeechSynthesisVoice * voice)
{
  if ([voice.identifier hasPrefix:@"com.apple.eloquence."])
    return MWMTTSVoiceGroupEloquence;
  if (@available(iOS 17.0, *))
    if ((voice.voiceTraits & AVSpeechSynthesisVoiceTraitIsNoveltyVoice) != 0)
      return MWMTTSVoiceGroupNovelty;
  return MWMTTSVoiceGroupStandard;
}

MWMTTSVoice * VoiceFromSpeechVoice(AVSpeechSynthesisVoice * voice)
{
  return [MWMTTSVoice voiceWithIdentifier:voice.identifier
                                    title:DisplayNameForVoice(voice)
                                   region:RegionNameForVoice(voice)
                                    group:GroupForVoice(voice)];
}

using Observer = id<MWMTextToSpeechObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMTTSLanguage ()

@property(nonatomic) NSString * code;
@property(nonatomic) NSString * title;

@end

@implementation MWMTTSLanguage

+ (instancetype)languageWithCode:(NSString *)code title:(NSString *)title
{
  return [[MWMTTSLanguage alloc] initLanguageWithCode:code title:title];
}

- (instancetype)initLanguageWithCode:(NSString *)code title:(NSString *)title
{
  self = [super init];
  if (self)
  {
    _code = code;
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
  return [self.code isEqualToString:language.code];
}

- (NSUInteger)hash
{
  return self.code.hash;
}

@end

@implementation MWMTTSVoice

+ (instancetype)voiceWithIdentifier:(NSString *)identifier
                              title:(NSString *)title
                             region:(NSString *)region
                              group:(MWMTTSVoiceGroup)group
{
  return [[MWMTTSVoice alloc] initVoiceWithIdentifier:identifier title:title region:region group:group];
}

- (instancetype)initVoiceWithIdentifier:(NSString *)identifier
                                  title:(NSString *)title
                                 region:(NSString *)region
                                  group:(MWMTTSVoiceGroup)group
{
  self = [super init];
  if (self)
  {
    _identifier = identifier;
    _title = title;
    _region = region;
    _group = group;
  }
  return self;
}

- (BOOL)isEqual:(id)object
{
  if (self == object)
    return YES;
  if (![object isKindOfClass:[MWMTTSVoice class]])
    return NO;
  MWMTTSVoice * voice = object;
  return [self.identifier isEqualToString:voice.identifier];
}

- (NSUInteger)hash
{
  return self.identifier.hash;
}

@end

// Everything derived from the installed voices: built on demand and dropped as a whole when they may
// have changed, so that its parts can never disagree.
@interface MWMTTSVoiceCatalog : NSObject

// BCP-47 tag of the system voice, e.g. "en-GB".
@property(nonatomic, readonly) NSString * deviceLanguageCode;

// Languages with at least one installed voice, in kLanguageList order, titled by their native names
// so that no locale lookup is needed.
- (std::vector<std::pair<std::string, std::string>> const &)languages;
- (NSArray<AVSpeechSynthesisVoice *> *)voicesReading:(NSString *)languageCode;
// The same voices as the pickers list them, see VoicesForLanguage.
- (NSArray<MWMTTSVoice *> *)listedVoicesReading:(NSString *)languageCode;

@end

@implementation MWMTTSVoiceCatalog
{
  NSDictionary<NSString *, NSArray<AVSpeechSynthesisVoice *> *> * _voicesByLanguage;
  NSMutableDictionary<NSString *, NSArray<MWMTTSVoice *> *> * _listedVoices;
  std::vector<std::pair<std::string, std::string>> _languages;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    // Neither +speechVoices nor +currentLanguageCode is an accessor: both rebuild their answer on
    // every call, tens of milliseconds worth. Ask each of them exactly once.
    _voicesByLanguage = VoicesByLanguage([AVSpeechSynthesisVoice speechVoices]);
    _deviceLanguageCode = [AVSpeechSynthesisVoice currentLanguageCode];
    _listedVoices = [NSMutableDictionary dictionary];
    for (auto const & [code, title] : routing::turns::sound::kLanguageList)
      if (_voicesByLanguage[@(std::string(code).c_str())] != nil)
        _languages.emplace_back(code, title);
  }
  return self;
}

- (std::vector<std::pair<std::string, std::string>> const &)languages
{
  return _languages;
}

- (NSArray<AVSpeechSynthesisVoice *> *)voicesReading:(NSString *)languageCode
{
  return _voicesByLanguage[languageCode] ?: @[];
}

// Deduplicating and sorting a language's voices costs more than the pickers should pay per row.
- (NSArray<MWMTTSVoice *> *)listedVoicesReading:(NSString *)languageCode
{
  NSArray<MWMTTSVoice *> * listed = _listedVoices[languageCode];
  if (!listed)
  {
    NSMutableArray<MWMTTSVoice *> * result = [NSMutableArray array];
    for (AVSpeechSynthesisVoice * voice in VoicesForLanguage([self voicesReading:languageCode], _deviceLanguageCode))
      [result addObject:VoiceFromSpeechVoice(voice)];
    listed = result;
    _listedVoices[languageCode] = listed;
  }
  return listed;
}

@end

@interface MWMTextToSpeech () <AVSpeechSynthesizerDelegate>

@property(nonatomic) AVSpeechSynthesizer * speechSynthesizer;
@property(nonatomic) AVSpeechSynthesisVoice * speechVoice;
@property(nonatomic) MWMTTSVoiceCatalog * catalog;
// The utterance -speakPreview: is playing or waiting to play, so that only a preview is interrupted
// by -stopPreview and only its end runs the completion.
@property(nonatomic) AVSpeechUtterance * previewUtterance;
// The last turn notification handed to the synthesizer, until it is spoken. A preview is only handed
// over once this is nil: stopSpeakingAtBoundary: drops the whole queue, so one waiting in it could
// neither be cancelled nor kept from delaying the next instruction.
@property(nonatomic) AVSpeechUtterance * notificationUtterance;
@property(nonatomic, copy) void (^previewCompletion)(void);
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
  // Voices can only be installed or removed while the app is in the background. Only drop the
  // catalog here - rebuilding it costs tens of milliseconds, and most activations never read it.
  auto tts = [self tts];
  tts.catalog = nil;
  tts.speechVoice = nil;
  [NSNotificationCenter.defaultCenter postNotificationName:MWMTextToSpeechVoicesDidChangeNotification object:nil];
}

- (MWMTTSVoiceCatalog *)catalog
{
  if (!_catalog)
    _catalog = [[MWMTTSVoiceCatalog alloc] init];
  return _catalog;
}

- (AVSpeechSynthesizer *)speechSynthesizer
{
  if (!_speechSynthesizer)
  {
    _speechSynthesizer = [[AVSpeechSynthesizer alloc] init];
    _speechSynthesizer.delegate = self;
  }
  return _speechSynthesizer;
}

// The locale to read the notifications in when the user has not chosen one: the first language they
// read the system in that OM can speak and has a voice for. +currentLanguageCode only covers the
// foremost one, so the rest of the preferred list is consulted after it.
- (NSString *)preferredLocale
{
  NSMutableArray<NSString *> * candidates =
      [@[[[self class] savedLanguage] ?: @"", self.catalog.deviceLanguageCode ?: @""] mutableCopy];
  [candidates addObjectsFromArray:NSLocale.preferredLanguages];

  auto const & languages = self.catalog.languages;
  for (NSString * candidate in candidates)
  {
    std::string const language = TwineLanguage(candidate);
    if (language.empty())
      continue;
    if (std::any_of(languages.begin(), languages.end(), [&language](auto const & l) { return l.first == language; }))
      return candidate;
  }
  return kDefaultLanguage;
}

- (instancetype)initTTS
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];

    [self setNotificationsLocale:[self preferredLocale]];

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
  _speechSynthesizer.delegate = nil;
}
// The saved voice is kept even when it does not read out `locale`: -createVoice: ignores it while it
// does not match, so switching away and back restores the user's choice instead of forgetting it.
- (void)setNotificationsLocale:(NSString *)locale
{
  [NSUserDefaults.standardUserDefaults setObject:locale forKey:kUserDefaultsTTSLanguageBcp47];
  [self createVoice:locale];
}

- (AVSpeechSynthesisVoice *)speechVoiceInUse
{
  // Recreate the voice if it was invalidated (applicationDidBecomeActive nils it to re-enumerate
  // installed voices) so callers such as the settings screen never observe a transient nil after the
  // app returns from the background.
  if (!self.speechVoice)
    [self createVoice:[[self class] savedLanguage]];
  return self.speechVoice;
}

// The synthesizer is created on demand and cannot fail, so a voice is all that is missing.
- (BOOL)isValid
{
  return _speechVoice != nil;
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
// Reports the language actually being read out, which is the fallback language rather than the saved
// one when the latter lost its last voice.
+ (MWMTTSLanguage *)currentLanguage
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  AVSpeechSynthesisVoice * voice = [tts speechVoiceInUse];
  return voice ? LanguageWithCode(TwineLanguage(voice.language)) : nil;
}
+ (NSArray<MWMTTSLanguage *> *)availableLanguages
{
  auto const & languages = [MWMTextToSpeech tts].catalog.languages;
  NSMutableArray<MWMTTSLanguage *> * result = [NSMutableArray arrayWithCapacity:languages.size()];
  for (auto const & [code, title] : languages)
    [result addObject:[MWMTTSLanguage languageWithCode:@(code.c_str()) title:@(title.c_str())]];
  return result;
}
+ (void)setLanguage:(MWMTTSLanguage *)language
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  AVSpeechSynthesisVoice * voice =
      BestVoiceForLanguage([tts.catalog voicesReading:language.code], tts.catalog.deviceLanguageCode);
  ASSERT(voice, ("A language is only listed when it has a voice", language.code.UTF8String));
  if (!voice)
    return;
  // Forget any pinned voice so that a better one installed later is picked up automatically.
  [self setSavedVoiceIdentifier:nil];
  [tts setNotificationsLocale:voice.language];
}
+ (NSArray<MWMTTSVoice *> *)voicesForLanguage:(MWMTTSLanguage *)language
{
  return [[MWMTextToSpeech tts].catalog listedVoicesReading:language.code];
}

+ (MWMTTSVoice *)bestVoiceForLanguage:(MWMTTSLanguage *)language
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  AVSpeechSynthesisVoice * voice =
      BestVoiceForLanguage([tts.catalog voicesReading:language.code], tts.catalog.deviceLanguageCode);
  return voice ? VoiceFromSpeechVoice(voice) : nil;
}

+ (MWMTTSVoice *)currentVoice
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  AVSpeechSynthesisVoice * voice = [tts speechVoiceInUse];
  if (!voice)
    return nil;
  // Report the entry the picker lists rather than the voice object in use: the picker keeps one
  // regional variant per voice name, and a saved voice may well be one of the others. Comparing the
  // two by identifier would then leave no row selected.
  NSString * languageCode = @(TwineLanguage(voice.language).c_str());
  for (AVSpeechSynthesisVoice * listed in VoicesForLanguage([tts.catalog voicesReading:languageCode],
                                                            tts.catalog.deviceLanguageCode))
    if ([listed.name isEqualToString:voice.name])
      return VoiceFromSpeechVoice(listed);
  return VoiceFromSpeechVoice(voice);
}

+ (void)setVoice:(MWMTTSVoice *)voice
{
  MWMTextToSpeech * tts = [MWMTextToSpeech tts];
  AVSpeechSynthesisVoice * speechVoice = [AVSpeechSynthesisVoice voiceWithIdentifier:voice.identifier];
  ASSERT(speechVoice, ("A voice is only offered while it is installed", voice.identifier.UTF8String));
  if (!speechVoice)
    return;
  // Store both in one step: the language a voice reads out is the language of the notifications.
  [self setSavedVoiceIdentifier:voice.identifier];
  [tts setNotificationsLocale:speechVoice.language];
}

+ (BOOL)isVoicePinned
{
  NSString * savedVoiceIdentifier = [self savedVoiceIdentifier];
  if (savedVoiceIdentifier.length == 0)
    return NO;
  // A saved voice that has been uninstalled or reads another language is not the one in use, and the
  // language is being read by its best voice as if nothing was pinned.
  AVSpeechSynthesisVoice * voice = [[MWMTextToSpeech tts] speechVoiceInUse];
  return [voice.identifier isEqualToString:savedVoiceIdentifier];
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

- (void)createVoice:(NSString *)locale
{
  // Prefer the saved voice while it is installed and still reads out `locale`. A regional variant
  // counts as a match (an en-AU voice keeps reading an en-US locale) because both produce the same
  // notification texts, whereas e.g. a zh-TW voice does not match a zh-CN locale.
  NSString * savedVoiceIdentifier = [[self class] savedVoiceIdentifier];
  AVSpeechSynthesisVoice * voice = nil;

  if (savedVoiceIdentifier.length > 0)
  {
    voice = [AVSpeechSynthesisVoice voiceWithIdentifier:savedVoiceIdentifier];
    if (voice && locale.length > 0 && TwineLanguage(voice.language) != TwineLanguage(locale))
      voice = nil;
  }

  if (!voice)
  {
    if (!locale)
      LOG(LWARNING, ("locale is nil. Trying default locale."));

    for (NSString * loc in locale ? @[locale, kDefaultLanguage] : @[kDefaultLanguage])
    {
      voice = BestVoiceForLanguage([self.catalog voicesReading:@(TwineLanguage(loc).c_str())], loc);
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
  self.notificationUtterance = utterance;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)playTurnNotifications:(NSArray<NSString *> *)turnNotifications
{
  auto stopSession = ^{
    // Reads the ivar: there is nothing to stop while no synthesizer has been created.
    if (self->_speechSynthesizer.isSpeaking)
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

  // Most location updates carry no notification at all; a preview may only be preempted by speech
  // that is actually about to start.
  if (turnNotifications.count == 0)
  {
    stopSession();
    return;
  }

  // A route instruction takes precedence over a voice being sampled in the settings.
  [self stopPreviewNotifying:YES];

  NSError * err = nil;
  if (![[AVAudioSession sharedInstance] setActive:YES error:&err])
  {
    LOG(LWARNING, ("Couldn't activate audio session: ", [err localizedDescription]));
    return;
  }

  for (NSString * notification in turnNotifications)
    [self speakOneString:notification];
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

- (void)speakPreview:(NSString *)text voiceIdentifier:(NSString *)voiceIdentifier completion:(void (^)(void))completion
{
  AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithIdentifier:voiceIdentifier];
  ASSERT(voice, ("A voice is only previewed while it is installed", voiceIdentifier.UTF8String));
  if (!voice || text.length == 0)
    return;

  [self stopPreview];

  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:text];
  utterance.voice = voice;
  utterance.rate = AVSpeechUtteranceDefaultSpeechRate;
  self.previewUtterance = utterance;
  self.previewCompletion = completion;
  // A preview waits out the instruction being spoken here rather than in the synthesizer's queue,
  // see -endUtterance:.
  if (!self.notificationUtterance)
    [self.speechSynthesizer speakUtterance:utterance];
}

- (void)stopPreview
{
  [self stopPreviewNotifying:NO];
}

// `notify` reports the preview as ended: an interruption by navigation still has to reach the
// settings screen, while the screen's own stop already knows.
- (void)stopPreviewNotifying:(BOOL)notify
{
  if (!self.previewUtterance)
    return;
  // A preview still waiting for a turn notification is not in the synthesizer's queue; stopping it
  // there would only cut the instruction short.
  BOOL const isPreviewSpeaking = self.notificationUtterance == nil;
  self.previewUtterance = nil;
  auto completion = self.previewCompletion;
  self.previewCompletion = nil;
  if (isPreviewSpeaking)
    [self.speechSynthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
  if (notify && completion)
    dispatch_async(dispatch_get_main_queue(), completion);
}

#pragma mark - AVSpeechSynthesizerDelegate

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance *)utterance
{
  [self endUtterance:utterance];
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didCancelSpeechUtterance:(AVSpeechUtterance *)utterance
{
  [self endUtterance:utterance];
}

- (void)endUtterance:(AVSpeechUtterance *)utterance
{
  if (utterance == self.notificationUtterance)
  {
    self.notificationUtterance = nil;
    // Nothing of ours is being spoken any more, so a preview held back by the instruction can go.
    if (self.previewUtterance)
      [self.speechSynthesizer speakUtterance:self.previewUtterance];
    return;
  }

  // Ignore the notifications spoken before the last one, and previews already superseded by a newer
  // one or by -stopPreview.
  if (utterance != self.previewUtterance)
    return;
  self.previewUtterance = nil;
  auto completion = self.previewCompletion;
  self.previewCompletion = nil;
  if (completion)
    dispatch_async(dispatch_get_main_queue(), completion);
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
  // Unknown identifiers have no localized name, and iOS 15 returns nil rather than an empty string.
  NSString * localizedName = [locale localizedStringForLocaleIdentifier:nsLocaleString];
  return localizedName ? std::string(localizedName.capitalizedString.UTF8String) : std::string();
}
}  // namespace tts
