#import "UIFont+MapsMeFonts.h"
#import "UITextView+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

@implementation UITextView (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText
{
  self.text = L(localizedText);
}

- (NSString *)localizedText
{
  return L(self.text);
}

- (void)setFontName:(NSString *)fontName
{
  self.font = [UIFont fontWithName:fontName];
}

- (void)setColorName:(NSString *)colorName
{
  self.textColor = [UIColor colorWithName:colorName];
}

- (void)setTintColorName:(NSString *)colorName
{
  self.tintColor = [UIColor colorWithName:colorName];
}

@end

@implementation MWMTextView (RuntimeAttributes)

- (void)setLocalizedPlaceholder:(NSString *)localizedPlaceholder
{
  self.placeholder = L(localizedPlaceholder);
}

- (NSString *)localizedPlaceholder
{
  return L(self.placeholder);
}

@end
