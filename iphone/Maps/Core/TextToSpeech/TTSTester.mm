#import "TTSTester.h"

#include "LocaleTranslator.h"

#include "base/logging.hpp"

@implementation TTSTester

static NSString * const NotFoundDelimiter = @"__not_found__";
// Keep one shuffled sequence per active language so previews cover every tip before repeating one.
static NSArray<NSString *> * g_testStrings;
static NSString * g_testStringsLanguage;
static NSUInteger g_testStringIndex;

- (NSString *)nextTestString:(NSString *)language
{
  if (!g_testStrings || ![language isEqualToString:g_testStringsLanguage])
  {
    g_testStrings = [self getTestStrings:language];
    if (!g_testStrings)
      return nil;
    g_testStringsLanguage = language;
    g_testStringIndex = 0;
  }

  NSString * result = g_testStrings[g_testStringIndex];
  if (++g_testStringIndex >= g_testStrings.count)
    g_testStringIndex = 0;
  return result;
}

- (NSArray<NSString *> *)getTestStrings:(NSString *)language
{
  NSString * twineLanguage = [NSString stringWithUTF8String:locale_translator::bcp47ToTwineLanguage(language).c_str()];
  // An unsupported tag has no language of ours to look up, and an empty resource name would resolve.
  if (twineLanguage.length == 0)
  {
    LOG(LWARNING, ("No twine language for ", language.UTF8String));
    return nil;
  }

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
