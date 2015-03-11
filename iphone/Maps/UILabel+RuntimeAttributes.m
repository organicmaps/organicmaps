//
//  UILabel+RuntimeAttributes.m
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "UILabel+RuntimeAttributes.h"
#import "UIKitCategories.h"

@implementation UILabel (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText {
  self.text = L(localizedText);
}

- (NSString *)localizedText {
  return L(self.text);
}

@end

@implementation UIButton (RuntimeAttributes)

- (void)setLocalizedText:(NSString *)localizedText {
  [self setTitle:L(localizedText) forState:UIControlStateNormal];
}

- (NSString *)localizedText {
  return L([self titleForState:UIControlStateNormal]);
}

@end
