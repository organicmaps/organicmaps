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
  UIView const * view = self.manager.ownerViewController.view;
  if ([view.subviews containsObject:self.extendedPlacePageView] && self.state != MWMiPhoneLandscapePlacePageStateClosed)
    return;

  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const height = size.width > size.height ? size.height : size.width;
  CGFloat const offset = height > kMaximumPlacePageWidth ? kMaximumPlacePageWidth : height;

  self.extendedPlacePageView.frame = CGRectMake(-offset, 0, offset, height);
  self.anchorImageView.image = nil;
  self.anchorImageView.backgroundColor = self.basePlacePageView.backgroundColor;
  self.actionBar = [MWMPlacePageActionBar actionBarForPlacePage:self];

#warning TODO (Vlad): Change 35. to computational constant.
  self.basePlacePageView.featureTable.contentInset = UIEdgeInsetsMake(0., 0., self.actionBar.height + 35., 0.);

  [view addSubview:self.extendedPlacePageView];
  self.actionBar.center = CGPointMake(self.actionBar.width / 2., height - self.actionBar.height / 2.);
  [self.extendedPlacePageView addSubview:self.actionBar];
}

- (void)show
{
  if (self.state != MWMiPhoneLandscapePlacePageStateOpen)
    self.state = MWMiPhoneLandscapePlacePageStateOpen;
}

- (void)dismiss
{
  self.state =  MWMiPhoneLandscapePlacePageStateClosed;
  [super dismiss];
}

- (void)addBookmark
{
  [self.basePlacePageView addBookmark];
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
    velocity.y = 20;
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

@end
