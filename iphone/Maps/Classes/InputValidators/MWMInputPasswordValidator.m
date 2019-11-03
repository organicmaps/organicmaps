#import "MWMInputPasswordValidator.h"

static NSUInteger const minPasswordLength = 8;

@implementation MWMInputPasswordValidator

- (BOOL)validateString:(NSString *)string
{
  if (![super validateString:string])
    return NO;
  return string.length >= minPasswordLength;
}

@end
