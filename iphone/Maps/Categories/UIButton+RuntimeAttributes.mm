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

- (void)setFontName:(NSString *)fontName { self.titleLabel.font = [UIFont fontWithName:fontName]; }
- (void)setTextColorName:(NSString *)colorName
{
  [self setTitleColor:[UIColor colorWithName:colorName] forState:UIControlStateNormal];
}

- (void)setTextColorHighlightedName:(NSString *)colorName
{
  [self setTitleColor:[UIColor colorWithName:colorName] forState:UIControlStateHighlighted];
}

- (void)setTextColorDisabledName:(NSString *)colorName
{
  [self setTitleColor:[UIColor colorWithName:colorName] forState:UIControlStateDisabled];
}

- (void)setTextColorSelectedName:(NSString *)colorName
{
  [self setTitleColor:[UIColor colorWithName:colorName] forState:UIControlStateSelected];
}

- (void)setBackgroundColorName:(NSString *)colorName
{
  objc_setAssociatedObject(self, @selector(backgroundColorName), colorName,
                           OBJC_ASSOCIATION_COPY_NONATOMIC);
  [self setBackgroundColor:[UIColor colorWithName:colorName] forState:UIControlStateNormal];
}

- (NSString *)backgroundColorName
{
  return objc_getAssociatedObject(self, @selector(backgroundColorName));
}

- (void)setBackgroundHighlightedColorName:(NSString *)colorName
{
  objc_setAssociatedObject(self, @selector(backgroundHighlightedColorName), colorName,
                           OBJC_ASSOCIATION_COPY_NONATOMIC);
  [self setBackgroundColor:[UIColor colorWithName:colorName] forState:UIControlStateHighlighted];
}

- (NSString *)backgroundHighlightedColorName
{
  return objc_getAssociatedObject(self, @selector(backgroundHighlightedColorName));
}

- (void)setBackgroundSelectedColorName:(NSString *)colorName
{
  objc_setAssociatedObject(self, @selector(backgroundSelectedColorName), colorName,
                           OBJC_ASSOCIATION_COPY_NONATOMIC);
  [self setBackgroundColor:[UIColor colorWithName:colorName] forState:UIControlStateSelected];
}

- (NSString *)backgroundSelectedColorName
{
  return objc_getAssociatedObject(self, @selector(backgroundSelectedColorName));
}

- (void)setBackgroundColor:(UIColor *)color forState:(UIControlState)state
{
  [self setBackgroundColor:UIColor.clearColor];
  [self setBackgroundImage:[UIImage imageWithColor:color] forState:state];
}

@end
