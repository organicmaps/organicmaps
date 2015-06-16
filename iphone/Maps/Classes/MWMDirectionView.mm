//
//  MWMDirectionView.m
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDirectionView.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"

static NSString * const kDirectionViewNibName = @"MWMDirectionView";
static CGFloat const kDirectionArrowSide = IPAD ? 260. : 160.;

@interface MWMDirectionView ()

@property (weak, nonatomic) UIViewController * ownerController;
@property (nonatomic) CGSize defaultSize;

@end

@implementation MWMDirectionView

+ (MWMDirectionView *)directionViewForViewController:(UIViewController *)viewController
{
  MWMDirectionView * view = [[[NSBundle mainBundle] loadNibNamed:kDirectionViewNibName owner:nil options:nil] firstObject];
  view.ownerController = viewController;
  view.directionArrow.size = CGSizeMake(kDirectionArrowSide, kDirectionArrowSide);
  view.directionArrow.image = [UIImage imageNamed:IPAD ? @"direction_big" : @"direction_mini"];
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate disableStandby];
  [view configure];
  return view;
}

- (void)configure
{
  NSString * const kFontName = @"HelveticaNeue";
  UIFont * titleFont = IPAD ? [UIFont fontWithName:kFontName size:52.] : [UIFont fontWithName:kFontName size:32.];
  UIFont * typeFont = IPAD ? [UIFont fontWithName:kFontName size:24.] : [UIFont fontWithName:kFontName size:16.];

  self.titleLabel.font = titleFont;
  self.distanceLabel.font = titleFont;
  self.typeLabel.font = typeFont;

  UIViewAutoresizing mask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  self.autoresizingMask = mask;
  self.contentView.autoresizingMask = mask;
  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;
  self.frame = self.ownerController.view.frame;
  [[[[UIApplication sharedApplication] delegate] window] addSubview:self];
}

- (void)layoutSubviews
{
  CGSize const size = [[UIScreen mainScreen] bounds].size;
  self.size = size;
  CGFloat const minimumBorderOffset = 40.;

  switch (self.ownerController.interfaceOrientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
    {
      CGFloat const defaultWidth = size.width - 3. * minimumBorderOffset - kDirectionArrowSide;
      [self resizeLabelsWithWidth:defaultWidth];

      CGFloat const titleOffset = 8.;
      CGFloat const typeOffset = 24.;
      CGFloat const contentViewHeight = size.height - 2. * minimumBorderOffset;
      CGFloat const contentViewOffset = (size.width - self.titleLabel.width - minimumBorderOffset - self.directionArrow.width) / 2.;
      CGFloat const contentViewWidth = self.titleLabel.width + minimumBorderOffset + self.directionArrow.width;
      self.contentView.frame = CGRectMake(contentViewOffset, minimumBorderOffset, contentViewWidth, contentViewHeight);
      self.directionArrow.center = CGPointMake(0., self.contentView.height / 2.);
      CGFloat const directionArrowOffsetX = self.directionArrow.maxX + minimumBorderOffset;
      CGFloat const actualLabelsBlockHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + typeOffset + self.distanceLabel.height;
      CGFloat const labelsBlockTopOffset = (contentViewHeight - actualLabelsBlockHeight) / 2.;
      self.titleLabel.origin = CGPointMake(directionArrowOffsetX, labelsBlockTopOffset);
      self.titleLabel.textAlignment = NSTextAlignmentLeft;
      self.typeLabel.origin = CGPointMake(directionArrowOffsetX, self.titleLabel.maxY + titleOffset);
      self.typeLabel.textAlignment = NSTextAlignmentLeft;
      self.distanceLabel.origin = CGPointMake(directionArrowOffsetX, self.typeLabel.maxY + typeOffset);
      self.distanceLabel.textAlignment = NSTextAlignmentLeft;
      break;
    }
    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
    {
      CGFloat const defaultWidth = size.width - 2. * minimumBorderOffset;
      [self resizeLabelsWithWidth:defaultWidth];

      CGFloat const titleOffset = IPAD ? 12. : 8.;
      CGFloat const arrowOffset = IPAD ? 80. : 40.;
      CGFloat const contentViewActualHeight = self.titleLabel.height + titleOffset + self.typeLabel.height + 2. * arrowOffset + kDirectionArrowSide + self.distanceLabel.height;
      CGFloat const contentViewSize = size.height > contentViewActualHeight ? contentViewActualHeight : size.height;
      CGFloat const yOffset = (size.height - contentViewSize) / 2.;
      self.contentView.frame = CGRectMake(minimumBorderOffset, yOffset, defaultWidth, contentViewSize);
      CGFloat xOffset = self.contentView.width / 2.;
      self.titleLabel.origin = CGPointMake(xOffset - self.titleLabel.width / 2., 0.);
      self.titleLabel.textAlignment = NSTextAlignmentCenter;
      self.typeLabel.origin = CGPointMake(xOffset - self.typeLabel.width / 2., self.titleLabel.maxY + titleOffset);
      self.typeLabel.textAlignment = NSTextAlignmentCenter;
      self.directionArrow.center = CGPointMake(xOffset, self.typeLabel.maxY + arrowOffset + kDirectionArrowSide / 2.);
      self.distanceLabel.origin = CGPointMake(xOffset - self.distanceLabel.width / 2., self.directionArrow.maxY + arrowOffset);
      self.distanceLabel.textAlignment = NSTextAlignmentCenter;
      break;
    }
    case UIInterfaceOrientationUnknown:
      break;
  }
}

- (void)resizeLabelsWithWidth:(CGFloat)width
{
  self.titleLabel.width = width;
  self.typeLabel.width = width;
  self.distanceLabel.width = width;
  [self.titleLabel sizeToFit];
  [self.typeLabel sizeToFit];
  [self.distanceLabel sizeToFit];
}

- (IBAction)tap:(UITapGestureRecognizer *)sender
{
  [self removeFromSuperview];
}

- (void)removeFromSuperview
{
  [(MapsAppDelegate *)[UIApplication sharedApplication].delegate enableStandby];
  [super removeFromSuperview];
}

@end
