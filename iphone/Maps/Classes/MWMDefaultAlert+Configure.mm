//
//  MWMRouteNotFoundDefaultAlert+Configure.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDefaultAlert+Configure.h"
#import "UIKitCategories.h"

@implementation MWMDefaultAlert (Configure)

- (void)configure {
  [self.messageLabel sizeToFit];
  [self configureViewSize];
}

- (void)configureViewSize {
  const CGFloat topMainViewOffset = 17.;
  const CGFloat minMainViewHeight = 144.;
  const CGFloat actualMainViewHeight = 2 * topMainViewOffset + self.messageLabel.height + self.okButton.height;
  self.height = actualMainViewHeight >= minMainViewHeight ? actualMainViewHeight : minMainViewHeight;
  self.messageLabel.minY = topMainViewOffset;
  self.deviderLine.minY = self.height - self.okButton.height;
  self.okButton.minY = self.deviderLine.minY + self.deviderLine.height;
}

@end
