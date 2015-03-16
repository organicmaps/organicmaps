//
//  MWMDownloadTransitMapAlert+Configure.m
//  Maps
//
//  Created by v.mikhaylenko on 12.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadTransitMapAlert+Configure.h"
#import "UIKitCategories.h"

@implementation MWMDownloadTransitMapAlert (Configure)

- (void)configure {
  [self.messageLabel sizeToFit];
  [self.titleLabel sizeToFit];
  [self.countryLabel sizeToFit];
  [self configureSpecsViewSize];
  [self configureMaintViewSize];
}

- (void)configureSpecsViewSize {
  const CGFloat topSpecsViewOffset = 16.;
  const CGFloat specsViewHeight = 2 * topSpecsViewOffset + self.countryLabel.frame.size.height;
  self.specsView.height = specsViewHeight;
  self.countryLabel.minY = topSpecsViewOffset;
  self.sizeLabel.center = CGPointMake(self.sizeLabel.center.x, self.countryLabel.center.y);
}

- (void)configureMaintViewSize {
  const CGFloat topMainViewOffset = 17.;
  const CGFloat secondMainViewOffset = 14.;
  const CGFloat thirdMainViewOffset = 20.;
  const CGFloat bottomMainViewOffset = 52.;
  const CGFloat mainViewHeight = topMainViewOffset + self.titleLabel.frame.size.height + secondMainViewOffset + self.messageLabel.frame.size.height + thirdMainViewOffset + self.specsView.frame.size.height + bottomMainViewOffset;
  self.height = mainViewHeight;
  self.titleLabel.minY = topMainViewOffset;
  self.messageLabel.minY = self.titleLabel.frame.origin.y + self.titleLabel.frame.size.height + secondMainViewOffset;
  self.specsView.minY = self.messageLabel.frame.origin.y + self.messageLabel.frame.size.height + thirdMainViewOffset;
  self.notNowButton.minY = self.specsView.frame.origin.y + self.specsView.frame.size.height;
  self.downloadButton.minY = self.notNowButton.frame.origin.y;
}


@end
