#import "MWMInputPasswordValidator.h"

NSUInteger constexpr minPasswordLength = 8;

@implementation MWMInputPasswordValidator

- (BOOL)validateString:(NSString *)string
{
  if (![super validateString:string])
    return NO;
  return string.length >= minPasswordLength;
}

@end
