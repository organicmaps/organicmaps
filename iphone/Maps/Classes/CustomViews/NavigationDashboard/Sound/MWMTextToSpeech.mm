//
//  MWMTextToSpeech.cpp
//  Maps
//
//  Created by vbykoianko on 10.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <AVFoundation/AVFoundation.h>
#import "Common.h"
#import "MWMTextToSpeech.h"

#include "Framework.h"

NSString * const MWMTEXTTOSPEECH_ENABLE = @"MWMTEXTTOSPEECH_ENABLE";
NSString * const MWMTEXTTOSPEECH_DISABLE = @"MWMTEXTTOSPEECH_DISABLE";

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
    // iOS has an issue with speechRate. AVSpeechUtteranceDefaultSpeechRate does not work correctly. It's a work around.
    self.speechRate = isIOSVersionLessThan(@"7.1.1") ? 0.3 : 0.15;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(enable)
                                                 name:MWMTEXTTOSPEECH_ENABLE
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(disable)
                                                 name:MWMTEXTTOSPEECH_DISABLE
                                               object:nil];
  }
  return self;
}

-(void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

+ (NSString *)twineFromBCP47:(NSString *)bcp47LangName
{
  NSAssert(bcp47LangName, @"bcp47LangName is nil");
  
  if ([bcp47LangName isEqualToString:@"zh-CN"] || [bcp47LangName isEqualToString:@"zh-CHS"]
      || [bcp47LangName isEqualToString:@"zh-SG"])
  {
    return @"zh-Hans"; // Chinese simplified
  }
  
  if ([bcp47LangName hasPrefix:@"zh"])
    return @"zh-Hant"; // Chinese traditional
  // Taking two first symbols of a language name. For example ru-RU -> ru
  return [bcp47LangName substringToIndex:2];
}

- (bool)isValid
{
  return _speechSynthesizer != nil && _speechVoice != nil;
}

- (void)enable
{
  if (YES == [self isEnable])
    return;
  
  if (![self isValid])
    [self createSynthesizer];
  
  GetFramework().EnableTurnNotifications(true);
}

- (void)disable
{
  if (NO == [self isEnable])
    return;
  
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
    locale = DEFAULT_LANG;
    NSAssert(locale, @"locale is nil. Trying default locale.");
  }
  
  NSArray * availTtsLangs = [AVSpeechSynthesisVoice speechVoices];
  
  if (!availTtsLangs)
  {
    NSAssert(availTtsLangs, @"availTtsLangs is nil. MWMTextToSpeech is not valid.");
    return; // self is not valid.
  }
  
  AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  if (!voice || ![availTtsLangs containsObject:voice])
  {
    locale = DEFAULT_LANG;
    AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
    if (!voice || ![availTtsLangs containsObject:voice])
    {
      NSAssert(availTtsLangs, @"Available language for TTS not found. MWMTextToSpeech is not valid.");
      return; // self is not valid.
    }
  }
  
  self.speechVoice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  GetFramework().SetTurnNotificationsLocale([[MWMTextToSpeech twineFromBCP47:locale] UTF8String]);
}

- (void)speakOneString:(NSString *)textToSpeak
{
  if (!textToSpeak || [textToSpeak length] == 0)
    return;
  
  NSLog(@"Speak text: %@", textToSpeak);
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = self.speechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)speak:(vector<string> const &)strings
{
  if (strings.empty())
    return;
  
  for (auto const & text : strings)
    [self speakOneString:@(text.c_str())];
}

- (void)playTurnNotifications
{
  if (![self isValid])
    return;
  
  Framework & frm = GetFramework();
  if (!frm.IsRoutingActive())
    return;
  
  vector<string> notifications;
  frm.GenerateTurnSound(notifications);
  if (notifications.empty())
    return;
  
  [self speak:notifications];
}

@end
