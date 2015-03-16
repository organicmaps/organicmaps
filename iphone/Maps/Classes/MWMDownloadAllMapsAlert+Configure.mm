//
//  MWMDownloadAllMapsAlert+Configure.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadAllMapsAlert+Configure.h"
#import "UIKitCategories.h"

@implementation MWMDownloadAllMapsAlert (Configure)

- (void)configure {
  [self.titleLabel sizeToFit];
  [self.messageLabel sizeToFit];
  [self configureMainViewSize];
}

- (void)configureMainViewSize {
  const CGFloat topMainViewOffset = 17.;
  const CGFloat secondMainViewOffset = 14.;
  const CGFloat thirdMainViewOffset = 20.;
  const CGFloat bottomMainViewOffset = 52.;
  const CGFloat mainViewHeight = topMainViewOffset + self.titleLabel.height + secondMainViewOffset + self.messageLabel.height + thirdMainViewOffset + self.specsView.height + bottomMainViewOffset;
  self.height = mainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.minY + self.titleLabel.height + secondMainViewOffset;
  self.specsView.minY = self.messageLabel.minY + self.messageLabel.height + thirdMainViewOffset;
  self.notNowButton.minY = self.specsView.minY + self.specsView.height;
}

@end
