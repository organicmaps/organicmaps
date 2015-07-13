//
//  MWMDownloadMapRequestView.m
//  Maps
//
//  Created by Ilya Grechuhin on 10.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloadMapRequest.h"
#import "MWMDownloadMapRequestView.h"
#import "UIKitCategories.h"

@interface MWMDownloadMapRequestView ()

@property (weak, nonatomic) IBOutlet UILabel * mapTitleLabel;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * verticalFreeSpace;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * bottomSpacing;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * unknownPositionLabelBottomOffset;
@property (weak, nonatomic) IBOutlet MWMDownloadMapRequest * owner;

@end

@implementation MWMDownloadMapRequestView

- (void)layoutSubviews
{
  [super layoutSubviews];
  UIView * superview = self.superview;
  BOOL const isLandscape = superview.height > superview.width;
  if (IPAD || isLandscape)
  {
    self.verticalFreeSpace.constant = 44.0;
    self.bottomSpacing.constant = 24.0;
    self.unknownPositionLabelBottomOffset.constant = 22.0;
  }
  else
  {
    self.verticalFreeSpace.constant = 20.0;
    self.bottomSpacing.constant = 8.0;
    self.unknownPositionLabelBottomOffset.constant = 18.0;
    CGFloat const iPhone6LandscapeHeight = 375.0;
    if (self.width < iPhone6LandscapeHeight)
    {
      self.mapTitleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
      self.mapTitleLabel.numberOfLines = 1;
    }
    else
    {
      self.mapTitleLabel.lineBreakMode = NSLineBreakByWordWrapping;
      self.mapTitleLabel.numberOfLines = 0;
    }
  }
  self.width = superview.width;
  self.minX = 0.0;
  if (self.minY > 0.0)
  {
    [UIView animateWithDuration:0.3 animations:^
    {
      [self move];
    }];
  }
  else
  {
    [self move];
  }
}

- (void)move
{
  if (self.owner.state == MWMDownloadMapRequestProgress)
  {
    UIView * superview = self.superview;
    BOOL const isLandscape = superview.height > superview.width;
    self.minY = (IPAD || isLandscape) ? 0.3 * self.superview.height : 0.0;
  }
  else
  {
    self.maxY = self.superview.height;
  }
}

@end
