#import "MWMInputValidator.h"

@implementation MWMInputValidator

- (BOOL)validateInput:(UITextField *)input
{
  return [self validateString:input.text];
}

- (BOOL)validateString:(NSString *)string
{
  return YES;
}

@end
