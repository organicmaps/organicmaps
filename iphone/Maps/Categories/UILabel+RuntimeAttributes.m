#import "UILabel+RuntimeAttributes.h"

// Runtime attributes for setting localized text in Xib.

@implementation UILabel (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText
{
  self.text = L(localizedText);
}
- (NSString *)localizedText
{
  NSString * text = self.text;
  return L(text);
}

@end
