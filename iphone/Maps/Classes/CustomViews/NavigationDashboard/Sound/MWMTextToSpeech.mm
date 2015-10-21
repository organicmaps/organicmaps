#import <AVFoundation/AVFoundation.h>
#import "Common.h"
#import "MWMTextToSpeech.h"

#include "Framework.h"

extern NSString * const kMwmTextToSpeechEnable = @"MWMTEXTTOSPEECH_ENABLE";
extern NSString * const kMwmTextToSpeechDisable = @"MWMTEXTTOSPEECH_DISABLE";

@interface MWMTextToSpeech()
@property (nonatomic) AVSpeechSynthesizer * speechSynthesizer;
@property (nonatomic) AVSpeechSynthesisVoice * speechVoice;
@property (nonatomic) float speechRate;
@end

@implementation MWMTextToSpeech

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    // Before 9.0 version iOS has an issue with speechRate. AVSpeechUtteranceDefaultSpeechRate does not work correctly.
    // It's a work around for iOS 7.x and 8.x.
    self.speechRate = isIOSVersionLessThan(@"7.1.1") ? 0.3 : (isIOSVersionLessThan(@"9.0.0") ? 0.15 : AVSpeechUtteranceDefaultSpeechRate);
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(enable)
                                                 name:kMwmTextToSpeechEnable
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(disable)
                                                 name:kMwmTextToSpeechDisable
                                               object:nil];
  }
  return self;
}

-(void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)isValid
{
  return _speechSynthesizer != nil && _speechVoice != nil;
}

- (void)enable
{
  if (![self isValid])
    [self createSynthesizer];
  
  GetFramework().EnableTurnNotifications(true);
}

- (void)disable
{
  GetFramework().EnableTurnNotifications(false);
}

- (BOOL)isEnable
{
  return GetFramework().AreTurnNotificationsEnabled() ? YES : NO;
}

- (void)createSynthesizer
{
  self.speechSynthesizer = [[AVSpeechSynthesizer alloc] init];
  
  // TODO(vbykoianko) Use [NSLocale preferredLanguages] instead of [AVSpeechSynthesisVoice currentLanguageCode].
  // [AVSpeechSynthesisVoice currentLanguageCode] is used now because of we need a language code in BCP-47.
  [self createVoice:[AVSpeechSynthesisVoice currentLanguageCode]];
}

- (void)createVoice:(NSString *)locale
{
  NSString * const DEFAULT_LANG = @"en-US";

  if (!locale)
  {
    LOG(LERROR, ("locale is nil. Trying default locale."));
    locale = DEFAULT_LANG;
  }
  
  NSArray * availTTSLangs = [AVSpeechSynthesisVoice speechVoices];
  
  if (!availTTSLangs)
  {
    LOG(LERROR, ("No languages for TTS. MWMTextFoSpeech is invalid."));
    return; // self is not valid.
  }
  
  AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  if (!voice || ![availTTSLangs containsObject:voice])
  {
    locale = DEFAULT_LANG;
    AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
    if (!voice || ![availTTSLangs containsObject:voice])
    {
      LOG(LERROR, ("The UI language and English are not available for TTS. MWMTestToSpeech is invalid."));
      return; // self is not valid.
    }
  }
  
  self.speechVoice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  NSString const * twineLang = bcp47ToTwineLanguage(locale);
  if (twineLang == nil)
  {
    LOG(LERROR, ("Cannot convert UI locale or default locale to twine language. MWMTestToSpeech is invalid."));
    return; // self is not valid.
  }
  GetFramework().SetTurnNotificationsLocale([twineLang UTF8String]);
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

@end
