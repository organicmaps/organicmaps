#import "MWMInputEmailValidator.h"

static NSString * const kEmailRegexPattern = @"[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,6}";

@implementation MWMInputEmailValidator

- (BOOL)validateString:(NSString *)string
{
  if (![super validateString:string])
    return NO;
  NSError * err;
  NSRegularExpression * regex =
      [NSRegularExpression regularExpressionWithPattern:kEmailRegexPattern
                                                options:NSRegularExpressionCaseInsensitive
                                                  error:&err];
  NSAssert(!err, @"Invalid regular expression");
  NSMutableArray<NSString *> * matches = [@[] mutableCopy];
  NSRange range = NSMakeRange(0, string.length);
  [regex enumerateMatchesInString:string
                          options:NSMatchingReportProgress
                            range:range
                       usingBlock:^(NSTextCheckingResult * _Nullable result, NSMatchingFlags flags,
                                    BOOL * _Nonnull stop)
  {
    [matches addObject:[string substringWithRange:result.range]];
  }];
  if (matches.count != 1)
    return NO;
  return [matches[0] isEqualToString:string];
}

@end
