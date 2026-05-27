#import "UITextField+RuntimeAttributes.h"

@implementation UITextField (RuntimeAttributes)

- (void)setLocalizedPlaceholder:(NSString *)placeholder
{
  self.placeholder = L(placeholder);
}

- (NSString *)localizedPlaceholder
{
  NSString * placeholder = self.placeholder;
  return L(placeholder);
}

@end
