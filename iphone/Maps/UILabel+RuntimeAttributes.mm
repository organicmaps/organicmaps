//
//  UILabel+RuntimeAttributes.m
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UILabel+RuntimeAttributes.h"
#import "UIKitCategories.h"

// Runtime attributes for setting localized text in Xib.

@implementation UILabel (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText {
  self.text = L(localizedText);
}

- (NSString *)localizedText {
  return L(self.text);
}

- (void)setFontName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  self.font = [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end

@implementation UIButton (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText {
  [self setTitle:L(localizedText) forState:UIControlStateNormal];
}

- (NSString *)localizedText {
  return L([self titleForState:UIControlStateNormal]);
}

- (void)setFontName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  self.titleLabel.font = [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end

@implementation UITextField (RuntimeAttributes)

- (void)setLocalizedPlaceholder:(NSString *)placeholder
{
  self.placeholder = L(placeholder);
}

- (NSString *)localizedPlaceholder
{
  return L(self.placeholder);
}

- (void)setFontName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  self.font = [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
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

- (void)setFontName:(NSString *)fontName
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  self.font = [[UIFont class] performSelector:NSSelectorFromString(fontName)];
#pragma clang diagnostic pop
}

@end
