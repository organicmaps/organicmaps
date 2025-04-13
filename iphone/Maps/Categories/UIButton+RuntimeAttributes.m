#import <objc/runtime.h>
#import "UIButton+RuntimeAttributes.h"

@implementation UIButton (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText
{
  [self setTitle:L(localizedText) forState:UIControlStateNormal];
  [self setTitle:L(localizedText) forState:UIControlStateDisabled];
}

- (NSString *)localizedText
{
  NSString * title = [self titleForState:UIControlStateNormal];
  return L(title);
}

@end
