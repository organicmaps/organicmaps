#import "MWMInputLoginValidator.h"

NSUInteger constexpr minLoginLength = 3;

@implementation MWMInputLoginValidator

- (BOOL)validateString:(NSString *)string
{
  if (![super validateString:string])
    return NO;
  return string.length >= minLoginLength;
}

@end
