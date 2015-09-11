//
//  MWMNextTurnPanel.m
//  Maps
//
//  Created by v.mikhaylenko on 24.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNextTurnPanel.h"
#import "UIKitCategories.h"

@interface MWMNextTurnPanel ()

@property (weak, nonatomic) IBOutlet UIView * contentView;
@property (weak, nonatomic) IBOutlet UIImageView * turnImage;

@end

@implementation MWMNextTurnPanel

+ (instancetype)turnPanelWithOwnerView:(UIView *)ownerView
{
  MWMNextTurnPanel * panel = [[[NSBundle mainBundle] loadNibNamed:[MWMNextTurnPanel className]
                                                            owner:nil
                                                          options:nil] firstObject];
  panel.parentView = ownerView;
  [ownerView insertSubview:panel atIndex:0];
  [panel configure];
  return panel;
}

- (void)configureWithImage:(UIImage *)image
{
  self.turnImage.image = image;
  if (self.hidden)
    self.hidden = NO;
}

- (void)configure
{
  if (IPAD)
  {
    self.backgroundColor = self.contentView.backgroundColor;
    self.contentView.backgroundColor = UIColor.clearColor;
  }
  self.autoresizingMask = UIViewAutoresizingFlexibleHeight;
}

@end
