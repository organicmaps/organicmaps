//
//  MWMiPhoneLandscapePlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 18.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageViewManager.h"
#import "UIKitCategories.h"
#import "MWMSpringAnimation.h"
#import "MWMPlacePage+Animation.h"

#include "Framework.h"

static CGFloat const kMaximumPlacePageWidth = 360.;
extern CGFloat const kBookmarkCellHeight;

typedef NS_ENUM(NSUInteger, MWMiPhoneLandscapePlacePageState)
{
  MWMiPhoneLandscapePlacePageStateClosed,
  MWMiPhoneLandscapePlacePageStateOpen
};

@interface MWMiPhoneLandscapePlacePage ()

@property (nonatomic) MWMiPhoneLandscapePlacePageState state;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) CGFloat panVelocity;

@end

@implementation MWMiPhoneLandscapePlacePage

- (void)configure
{
  [super configure];
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = MIN(size.width, size.height);
  self.anchorImageView.backgroundColor = [UIColor whiteColor];
  self.anchorImageView.image = nil;
  CGFloat const headerViewHeight = self.basePlacePageView.height - self.basePlacePageView.featureTable.height;
  CGFloat const tableContentHeight = self.basePlacePageView.featureTable.contentSize.height;
  self.basePlacePageView.featureTable.contentInset = UIEdgeInsetsMake(0., 0., self.actionBar.height + (tableContentHeight - height - headerViewHeight + self.anchorImageView.height), 0.);
  [self.extendedPlacePageView addSubview:self.actionBar];
  [self.manager addSubviews:@[self.extendedPlacePageView] withNavigationController:nil];
}

- (void)show
{
  self.state = MWMiPhoneLandscapePlacePageStateOpen;
}

- (void)dismiss
{
  self.state = MWMiPhoneLandscapePlacePageStateClosed;
}

- (void)updateTargetPoint
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = MIN(size.width, size.height);
  CGFloat const offset = MIN(height, kMaximumPlacePageWidth);
  switch (self.state)
  {
    case MWMiPhoneLandscapePlacePageStateClosed:
      [self.actionBar removeFromSuperview];
      self.targetPoint = CGPointMake(-offset / 2., (height + self.topBound) / 2.);
      break;
    case MWMiPhoneLandscapePlacePageStateOpen:
      self.targetPoint = CGPointMake(offset / 2., (height + self.topBound) / 2.);
      break;
  }
}
#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  UIView * ppv = self.extendedPlacePageView;
  UIView * ppvSuper = ppv.superview;
  ppv.minX += [sender translationInView:ppvSuper].x;
  ppv.minX = MIN(ppv.minX, 0.0);
  [sender setTranslation:CGPointZero inView:ppvSuper];
  [self cancelSpringAnimation];
  UIGestureRecognizerState const state = sender.state;
  if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled)
  {
    sender.enabled = NO;
    self.panVelocity = [sender velocityInView:ppvSuper].x;
    if (self.panVelocity > 0)
      [self show];
    else
      [self.manager dismissPlacePage];
  }
}

- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  CGFloat const statusBarHeight = [[UIApplication sharedApplication] statusBarFrame].size.height;
  MWMBasePlacePageView const * basePlacePageView = self.basePlacePageView;
  UITableView const * tableView = basePlacePageView.featureTable;
  CGFloat const baseViewHeight = basePlacePageView.height;
  CGFloat const tableHeight = tableView.contentSize.height;
  CGFloat const headerViewHeight = baseViewHeight - tableHeight;
  CGFloat const titleOriginY = tableHeight - kBookmarkCellHeight - tableView.contentOffset.y;

  [UIView animateWithDuration:0.3f animations:^
  {
    self.basePlacePageView.transform = CGAffineTransformMakeTranslation(0., statusBarHeight - headerViewHeight - titleOriginY);
  }];
}

- (void)willFinishEditingBookmarkTitle:(NSString *)title
{
  [super willFinishEditingBookmarkTitle:title];
  [UIView animateWithDuration:0.3f animations:^
  {
    self.basePlacePageView.transform = CGAffineTransformMakeTranslation(0., 0.);
  }];
}

#pragma mark - Properties

- (void)setState:(MWMiPhoneLandscapePlacePageState)state
{
  _state = state;
  [self updateTargetPoint];
}

- (void)setTopBound:(CGFloat)topBound
{
  super.topBound = topBound;
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = MIN(size.width, size.height);
  CGFloat const offset = MIN(height, kMaximumPlacePageWidth);
  self.extendedPlacePageView.frame = CGRectMake(0., topBound, offset, height - topBound);
  self.actionBar.width = offset;
  self.actionBar.minX = 0.0;
  self.actionBar.maxY = height - topBound;
  [self updateTargetPoint];
}

- (void)setTargetPoint:(CGPoint)targetPoint
{
  _targetPoint = targetPoint;
  __weak MWMiPhoneLandscapePlacePage * weakSelf = self;
  [self startAnimatingPlacePage:self initialVelocity:CGPointMake(self.panVelocity, 0.0) completion:^
  {
    __strong MWMiPhoneLandscapePlacePage * self = weakSelf;
    if (self.state == MWMiPhoneLandscapePlacePageStateClosed)
      [super dismiss];
    else
      self.panRecognizer.enabled = YES;
  }];
  self.panVelocity = 0.0;
}

@end
