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

- (void)setFontName:(NSString *)fontName
{
  self.font = [UIFont fontWithName:fontName];
}

- (void)setColorName:(NSString *)colorName
{
  self.textColor = [UIColor colorWithName:colorName];
}

@end
