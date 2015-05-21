//
//  MWMPlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 17.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMiPhonePortraitPlacePage.h"
#import "MWMSpringAnimation.h"
#import "UIKitCategories.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePage+Animation.h"

#include "Framework.h"

typedef NS_ENUM(NSUInteger, MWMiPhonePortraitPlacePageState)
{
  MWMiPhonePortraitPlacePageStateClosed,
  MWMiPhonePortraitPlacePageStatePreview,
  MWMiPhonePortraitPlacePageStateOpen
};

@interface MWMiPhonePortraitPlacePage ()

@property (nonatomic) MWMiPhonePortraitPlacePageState state;
@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) MWMPlacePageNavigationBar *navigationBar;

@end

@implementation MWMiPhonePortraitPlacePage

- (void)configure
{
  [super configure];
  UIView const * view = self.manager.ownerViewController.view;
  if ([view.subviews containsObject:self.extendedPlacePageView])
  {
    self.state = MWMiPhonePortraitPlacePageStatePreview;
    return;
  }

  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = size.width > size.height ? size.height : size.width;
  CGFloat const height = size.width > size.height ? size.width : size.height;
  self.extendedPlacePageView.frame = CGRectMake(0., height, width, height);
  [view addSubview:self.extendedPlacePageView];
  self.actionBar = [MWMPlacePageActionBar actionBarForPlacePage:self];
  self.actionBar.center = CGPointMake(width / 2., height + self.actionBar.height / 2.);
  [view addSubview:self.actionBar];
  self.state = MWMiPhonePortraitPlacePageStatePreview;
  [UIView animateWithDuration:0.25f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
  {
    self.actionBar.center = CGPointMake(width / 2., height - self.actionBar.height / 2.);
  }
  completion:nil];
}

- (void)show
{
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)dismiss
{
  UIView const * view = self.manager.ownerViewController.view;
  self.state =  MWMiPhonePortraitPlacePageStateClosed;
  [UIView animateWithDuration:.25f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
   {
    self.actionBar.center = CGPointMake(view.midX, view.maxY + self.actionBar.height / 2.);
  }
  completion:^(BOOL finished)
  {
    [self.actionBar removeFromSuperview];
    self.actionBar = nil;
    [self.navigationBar removeFromSuperview];
    self.navigationBar = nil;
    [super dismiss];
  }];
}

- (void)addBookmark
{
  [self.basePlacePageView addBookmark];
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)setState:(MWMiPhonePortraitPlacePageState)state
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = size.width > size.height ? size.height : size.width;
  CGFloat const height = size.width > size.height ? size.width : size.height;
  switch (state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
      GetFramework().GetBalloonManager().RemovePin();
      self.targetPoint = CGPointMake(self.extendedPlacePageView.width / 2., self.extendedPlacePageView.height * 2.);
      break;

    case MWMiPhonePortraitPlacePageStatePreview:
    {
      static CGFloat const kPreviewPlacePageOffset = 30.;

      CGFloat const h = height / 2. - (self.basePlacePageView.titleLabel.height + kPreviewPlacePageOffset + self.basePlacePageView.typeLabel.height + self.actionBar.height);
      self.targetPoint = CGPointMake(width / 2., height + h);
      if (self.navigationBar)
      {
        [UIView animateWithDuration:.25f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
         {
          self.navigationBar.transform = CGAffineTransformMakeTranslation(0., - self.navigationBar.height);
        }
         completion:^(BOOL finished)
        {
          [self.navigationBar removeFromSuperview];
        }];
      }
      break;
    }

    case MWMiPhonePortraitPlacePageStateOpen:
    {
      static CGFloat const kOpenPlacePageOffset = 27.;
      CGFloat const h = height / 2. - (self.basePlacePageView.titleLabel.height + kOpenPlacePageOffset + self.basePlacePageView.typeLabel.height + self.basePlacePageView.featureTable.height + self.actionBar.height);
      self.targetPoint = CGPointMake(width / 2., height + h);
      if (self.targetPoint.y <= height / 2.)
      {
        self.navigationBar = [MWMPlacePageNavigationBar navigationBarWithPlacePage:self];
        [UIView animateWithDuration:0.25f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
          {
          self.navigationBar.transform = CGAffineTransformMakeTranslation(0., self.navigationBar.height);
        }
        completion:nil];
      }
      break;
    }
  }
  _state = state;
  [self startAnimatingPlacePage:self initialVelocity:self.springAnimation.velocity];
}

#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  CGPoint const point = [sender translationInView:self.extendedPlacePageView.superview];
  self.extendedPlacePageView.center = CGPointMake(self.extendedPlacePageView.center.x, self.extendedPlacePageView.center.y + point.y);
  [sender setTranslation:CGPointZero inView:self.extendedPlacePageView.superview];
  if (sender.state == UIGestureRecognizerStateEnded)
  {
    CGPoint velocity = [sender velocityInView:self.extendedPlacePageView.superview];
    velocity.x = 5;
    self.state = velocity.y >= 0. ? MWMiPhonePortraitPlacePageStateClosed : MWMiPhonePortraitPlacePageStateOpen;
    [self startAnimatingPlacePage:self initialVelocity:velocity];
    if (self.state == MWMiPhonePortraitPlacePageStateClosed)
    {
      self.actionBar.alpha = 0.;
      [self.manager dismissPlacePage];
    }
  }
  else
  {
    [self cancelSpringAnimation];
  }
}

- (IBAction)didTap:(UITapGestureRecognizer *)sender
{
  switch (self.state)
  {
      case MWMiPhonePortraitPlacePageStateClosed:
        self.state = MWMiPhonePortraitPlacePageStatePreview;
        break;

      case MWMiPhonePortraitPlacePageStatePreview:
        self.state = MWMiPhonePortraitPlacePageStateOpen;
        break;

      case MWMiPhonePortraitPlacePageStateOpen:
        self.state = MWMiPhonePortraitPlacePageStatePreview;
        break;
  }
  [self startAnimatingPlacePage:self initialVelocity:self.springAnimation.velocity];
}

@end
