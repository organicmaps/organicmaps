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
    self.speechSynthesizer = [[AVSpeechSynthesizer alloc] init];

    // TODO(vbykoianko) Use [NSLocale preferredLanguages] instead of [AVSpeechSynthesisVoice currentLanguageCode].
    // [AVSpeechSynthesisVoice currentLanguageCode] is used now because of we need a language code in BCP-47.
    [self setLocaleIfAvailable:[AVSpeechSynthesisVoice currentLanguageCode]];
    // iOS has an issue with speechRate. AVSpeechUtteranceDefaultSpeechRate does not work correctly. It's a work around.
    self.speechRate = isIOSVersionLessThan(@"7.1.1") ? 0.3 : 0.15;
  }
  return self;
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

- (void)setLocaleIfAvailable:(NSString *)locale
{
  NSAssert(locale, @"locale is nil");
  
  NSArray * availTtsLangs = [AVSpeechSynthesisVoice speechVoices];
  NSAssert(availTtsLangs, @"availTtsLangs is nil");
  
  AVSpeechSynthesisVoice * voice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  if(!voice || ![availTtsLangs containsObject:voice])
    locale = @"en-GB";
  
  self.speechVoice = [AVSpeechSynthesisVoice voiceWithLanguage:locale];
  GetFramework().SetTurnNotificationsLocale([[MWMTextToSpeech twineFromBCP47:locale] UTF8String]);
}

- (void)speakText:(NSString *)textToSpeak
{
  if (!textToSpeak)
    return;
  
  NSLog(@"Speak text: %@", textToSpeak);
  AVSpeechUtterance * utterance = [AVSpeechUtterance speechUtteranceWithString:textToSpeak];
  utterance.voice = self.speechVoice;
  utterance.rate = self.speechRate;
  [self.speechSynthesizer speakUtterance:utterance];
}

- (void)speakNotifications:(vector<string> const &)turnNotifications
{
  if (turnNotifications.empty())
    return;
  
  for (auto const & text : turnNotifications)
    [self speakText:@(text.c_str())];
}

@end
