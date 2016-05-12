#import "Common.h"
#import "MWMTextToSpeech.h"
#import "Statistics.h"
#import <AVFoundation/AVFoundation.h>

#include "LocaleTranslator.h"

#include "Framework.h"
#include "sound/tts/languages.hpp"

extern NSString * const kUserDefaultsTTSLanguageBcp47 = @"UserDefaultsTTSLanguageBcp47";
extern NSString * const kUserDafaultsNeedToEnableTTS = @"UserDefaultsNeedToEnableTTS";
static NSString * const DEFAULT_LANG = @"en-US";

using namespace locale_translator;

@interface MWMTextToSpeech()
{
  vector<pair<string, string>> _availableLanguages;
}

@property (nonatomic) AVSpeechSynthesizer * speechSynthesizer;
@property (nonatomic) AVSpeechSynthesisVoice * speechVoice;
@property (nonatomic) float speechRate;

@end

@implementation MWMTextToSpeech

+ (instancetype)tts
{
  static dispatch_once_t onceToken;
  static MWMTextToSpeech * tts = nil;
  dispatch_once(&onceToken, ^
  {
    tts = [[super alloc] initTTS];
  });
  return tts;
}

- (instancetype)initTTS
{
  self = [super init];
  if (self)
  {
    _availableLanguages = availableLanguages();

    NSString * saved = self.savedLanguage;
    NSString * preferedLanguageBcp47;
    if (saved.length)
      preferedLanguageBcp47 = saved;
    else
      preferedLanguageBcp47 = [AVSpeechSynthesisVoice currentLanguageCode];

    pair<string, string> const lan =
        make_pair([preferedLanguageBcp47 UTF8String],
                  tts::translatedTwine(bcp47ToTwineLanguage(preferedLanguageBcp47)));

    if (find(_availableLanguages.begin(), _availableLanguages.end(), lan) != _availableLanguages.end())
      [self setNotificationsLocale:preferedLanguageBcp47];
    else
      [self setNotificationsLocale:DEFAULT_LANG];

    // Before 9.0 version iOS has an issue with speechRate. AVSpeechUtteranceDefaultSpeechRate does not work correctly.
    // It's a work around for iOS 7.x and 8.x.
    _speechRate = isIOSVersionLessThan(@"7.1.1") ? 0.3 : (isIOSVersionLessThan(@"9.0.0") ? 0.15 : AVSpeechUtteranceDefaultSpeechRate);
  }
  return self;
}

+ (void)activateAudioSession
{
  NSError * err = nil;
  AVAudioSession * audioSession = [AVAudioSession sharedInstance];
  if (![audioSession setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:&err])
  {
    LOG(LWARNING, ("[ setCategory]] error.", [err localizedDescription]));
    return;
  }
  if (![audioSession setActive:YES error:&err])
    LOG(LWARNING, ("[[AVAudioSession sharedInstance] setActive]] error.", [err localizedDescription]));
}

- (vector<pair<string, string>>)availableLanguages
{
  return _availableLanguages;
}

- (void)setNotificationsLocale:(NSString *)locale
{
  [Statistics logEvent:kStatEventName(kStatTTSSettings, kStatChangeLanguage)
                   withParameters:@{kStatValue : locale}];
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:locale forKey:kUserDefaultsTTSLanguageBcp47];
  [ud synchronize];
  [self createVoice:locale];
}

- (BOOL)isValid
{
  return _speechSynthesizer != nil && _speechVoice != nil;
}

- (BOOL)isNeedToEnable
{
  return [[NSUserDefaults standardUserDefaults] boolForKey:kUserDafaultsNeedToEnableTTS];
}

- (void)setNeedToEnable:(BOOL)need
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:need forKey:kUserDafaultsNeedToEnableTTS];
  [ud synchronize];
}

- (void)enable
{
  [self setNeedToEnable:YES];
  if (![self isValid])
    [self createSynthesizer];
  GetFramework().EnableTurnNotifications(true);
}

- (void)disable
{
  [self setNeedToEnable:NO];
  GetFramework().EnableTurnNotifications(false);
}

- (BOOL)isEnable
{
  return GetFramework().AreTurnNotificationsEnabled() ? YES : NO;
}

- (NSString *)savedLanguage
{
  return [[NSUserDefaults standardUserDefaults] stringForKey:kUserDefaultsTTSLanguageBcp47];
}

- (void)createSynthesizer
{
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^
  {
    self.speechSynthesizer = [[AVSpeechSynthesizer alloc] init];
    [self createVoice:self.savedLanguage];
  });
  // TODO(vbykoianko) Use [NSLocale preferredLanguages] instead of [AVSpeechSynthesisVoice currentLanguageCode].
  // [AVSpeechSynthesisVoice currentLanguageCode] is used now because of we need a language code in BCP-47.
}

- (void)createVoice:(NSString *)locale
{
  NSMutableArray<NSString *> * candidateLocales = [@[DEFAULT_LANG, @"en-GB"] mutableCopy];

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
      LOG(LERROR, ("Cannot convert UI locale or default locale to twine language. MWMTextToSpeech is invalid."));
    else
      GetFramework().SetTurnNotificationsLocale(twineLang);
  }
  else
  {
    LOG(LERROR, ("The UI language and English are not available for TTS. MWMTextToSpeech is invalid."));
  }
}

- (void)speakOneString:(NSString *)textToSpeak
{
  if (!textToSpeak || ![textToSpeak length])
    return;
  
  NSLog(@"Speak text: %@", textToSpeak);
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = self.speechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)playTurnNotifications
{
  Framework & frm = GetFramework();
  if (!frm.IsRoutingActive())
    return;
  
  vector<string> notifications;
  frm.GenerateTurnNotifications(notifications);

  if (![self isValid])
    return;

  for (auto const & text : notifications)
    [self speakOneString:@(text.c_str())];
}

static vector<pair<string, string>> availableLanguages()
{
  NSArray<AVSpeechSynthesisVoice *> * voices = [AVSpeechSynthesisVoice speechVoices];
  vector<pair<string, string>> native(voices.count);
  for (AVSpeechSynthesisVoice * v in voices)
    native.emplace_back(make_pair(bcp47ToTwineLanguage(v.language), [v.language UTF8String]));

  using namespace routing::turns::sound;
  vector<pair<string, string>> result;
  for (auto const & p : kLanguageList)
  {
    for (pair<string, string> const & lang : native)
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

@end


namespace tts
{

string translatedTwine(string const & twine)
{
  auto const & list = routing::turns::sound::kLanguageList;
  auto const it = find_if(list.begin(), list.end(), [&twine](pair<string, string> const & pair)
                          {
                            return pair.first == twine;
                          });
  
  if (it != list.end())
    return it->second;
  else
    return "";
}
} // namespace tts
