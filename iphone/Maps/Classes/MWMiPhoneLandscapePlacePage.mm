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

@end

@implementation MWMiPhoneLandscapePlacePage

- (void)configure
{
  [super configure];

  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = size.width > size.height ? size.height : size.width;
  CGFloat const offset = height > kMaximumPlacePageWidth ? kMaximumPlacePageWidth : height;

  UIView const * view = self.manager.ownerViewController.view;
  if ([view.subviews containsObject:self.extendedPlacePageView] && self.state != MWMiPhoneLandscapePlacePageStateClosed)
    return;
  
  self.extendedPlacePageView.frame = CGRectMake(0., 0., offset, height);
  
  self.anchorImageView.backgroundColor = [UIColor whiteColor];
  self.anchorImageView.image = nil;
  CGFloat const headerViewHeight = self.basePlacePageView.height - self.basePlacePageView.featureTable.height;
  CGFloat const tableContentHeight = self.basePlacePageView.featureTable.contentSize.height;
  self.basePlacePageView.featureTable.contentInset = UIEdgeInsetsMake(0., 0., self.actionBar.height+ (tableContentHeight - height - headerViewHeight + self.anchorImageView.height), 0.);
  [view addSubview:self.extendedPlacePageView];
  self.actionBar.width = offset;
  self.actionBar.center = CGPointMake(self.actionBar.width / 2., height - self.actionBar.height / 2.);
  [self.extendedPlacePageView addSubview:self.actionBar];
}

- (void)show
{
  self.state = MWMiPhoneLandscapePlacePageStateOpen;
}

- (void)setState:(MWMiPhoneLandscapePlacePageState)state
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = size.width > size.height ? size.height : size.width;
  CGFloat const offset = height > kMaximumPlacePageWidth ? kMaximumPlacePageWidth : height;

  switch (state)
  {
    case MWMiPhoneLandscapePlacePageStateClosed:
      GetFramework().GetBalloonManager().RemovePin();
      self.targetPoint = CGPointMake(-offset / 2., height / 2.);
      break;

    case MWMiPhoneLandscapePlacePageStateOpen:
      self.targetPoint = CGPointMake(offset / 2., height / 2.);
      break;
  }
  _state = state;
  [self startAnimatingPlacePage:self initialVelocity:self.springAnimation.velocity];
}

#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  CGPoint const point = [sender translationInView:self.extendedPlacePageView.superview];
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = size.width > size.height ? size.height : size.width;
  CGFloat const offset = height > kMaximumPlacePageWidth ? kMaximumPlacePageWidth : height;
  CGFloat const x = self.extendedPlacePageView.center.x + point.x;

  if (x > offset / 2.)
    return;

  self.extendedPlacePageView.center = CGPointMake(self.extendedPlacePageView.center.x + point.x, self.extendedPlacePageView.center.y );
  [sender setTranslation:CGPointZero inView:self.extendedPlacePageView.superview];
  if (sender.state == UIGestureRecognizerStateEnded)
  {
    CGPoint velocity = [sender velocityInView:self.extendedPlacePageView.superview];
    velocity.y = 5.;
    self.state = velocity.x < 0. ? MWMiPhoneLandscapePlacePageStateClosed : MWMiPhoneLandscapePlacePageStateOpen;
    [self startAnimatingPlacePage:self initialVelocity:velocity];
    if (self.state == MWMiPhoneLandscapePlacePageStateClosed)
      [self.manager dismissPlacePage];
  }
  else
  {
    [self cancelSpringAnimation];
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

- (void)willFinishEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  [UIView animateWithDuration:0.3f animations:^
  {
    self.basePlacePageView.transform = CGAffineTransformMakeTranslation(0., 0.);
  }];
}

@end
