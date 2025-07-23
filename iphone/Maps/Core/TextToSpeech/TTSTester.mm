#import "TTSTester.h"

#include "LocaleTranslator.h"
#include "MWMTextToSpeech.h"

#include "base/logging.hpp"

@implementation TTSTester

static NSString * const NotFoundDelimiter = @"__not_found__";

NSArray<NSString *> * testStrings;
NSString * testStringsLanguage;

int testStringIndex;

- (void)playRandomTestString
{
  NSString * currentTTSLanguage = MWMTextToSpeech.savedLanguage;
  if (testStrings == nil || ![currentTTSLanguage isEqualToString:testStringsLanguage])
  {
    testStrings = [self getTestStrings:currentTTSLanguage];
    if (testStrings == nil)
    {
      LOG(LWARNING, ("Couldn't load TTS test strings"));
      return;
    }
    testStringsLanguage = currentTTSLanguage;
  }

  [[MWMTextToSpeech tts] play:testStrings[testStringIndex]];

  if (++testStringIndex >= testStrings.count)
    testStringIndex = 0;
}

- (NSArray<NSString *> *)getTestStrings:(NSString *)language
{
  NSString * twineLanguage = [NSString stringWithUTF8String:locale_translator::bcp47ToTwineLanguage(language).c_str()];
  NSString * languagePath = [NSBundle.mainBundle pathForResource:twineLanguage ofType:@"lproj"];
  if (languagePath == nil)
  {
    LOG(LWARNING, ("Couldn't find translation file for ", twineLanguage.UTF8String));
    return nil;
  }
  NSBundle * bundle = [NSBundle bundleWithPath:languagePath];

  NSMutableArray * appTips = [NSMutableArray new];
  for (int idx = 0;; idx++)
  {
    NSString * appTipKey = [NSString stringWithFormat:@"app_tip_%02d", idx];
    NSString * appTip = [bundle localizedStringForKey:appTipKey value:NotFoundDelimiter table:nil];
    if ([appTip isEqualToString:NotFoundDelimiter])
      break;
    [appTips addObject:appTip];
  }

  // shuffle
  for (NSUInteger i = appTips.count; i > 1; i--)
    [appTips exchangeObjectAtIndex:i - 1 withObjectAtIndex:arc4random_uniform((u_int32_t)i)];

  return appTips;
}

@end
