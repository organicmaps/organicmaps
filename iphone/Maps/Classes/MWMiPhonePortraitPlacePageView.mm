//
//  MWMPlacePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 17.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMiPhonePortraitPlacePageView.h"
#import "MWMSpringAnimation.h"
#import "UIKitCategories.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageNavigationBar.h"
#import "MWMPlacePageViewManager.h"

#include "Framework.h"

@interface MWMiPhonePortraitPlacePageView ()

@property (nonatomic) CGPoint targetPoint;
@property (nonatomic) MWMSpringAnimation *springAnimation;
@property (weak, nonatomic) UIViewController *ownerViewController;
@property (nonatomic) MWMPlacePageNavigationBar *navBar;

@end

@implementation MWMiPhonePortraitPlacePageView

- (void)show
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  UIView const *view = self.placePageManager.ownerViewController.view;
  self.frame = CGRectMake(0., size.height, size.width, size.height);
  self.actionBar = [MWMPlacePageActionBar actionBarForPlacePage:self];
  [view addSubview:self];
  [view addSubview:self.actionBar];
  self.actionBar.center = CGPointMake(view.midX, view.maxY + self.actionBar.height / 2.);

  [UIView animateKeyframesWithDuration:.25f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
  {
    self.actionBar.center = CGPointMake(view.midX, view.maxY - self.actionBar.height / 2.);
  }
  completion:nil];
}

- (CGPoint)targetPoint
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  switch (self.state)
  {
    case MWMPlacePageStateClosed:
      GetFramework().GetBalloonManager().RemovePin();
      _targetPoint = CGPointMake(size.width / 2., size.height * 2.);
      if (self.navBar)
      {
        [UIView animateWithDuration:.3f animations:^
        {
          self.navBar.transform = CGAffineTransformMakeTranslation(0., - self.navBar.height);
        }
        completion:^(BOOL finished)
        {
          [self.navBar removeFromSuperview];
        }];
      }
      break;

    case MWMPlacePageStatePreview:
    {
      static CGFloat const kPreviewPlacePageOffset = 30.;

      CGFloat const h = size.height / 2. - (self.titleLabel.height + kPreviewPlacePageOffset + self.typeLabel.height + self.actionBar.height);
      _targetPoint = CGPointMake(size.width / 2., size.height + h);
      if (self.navBar)
      {
        [UIView animateWithDuration:.3f animations:^
        {
          self.navBar.transform = CGAffineTransformMakeTranslation(0., - self.navBar.height);
        } completion:^(BOOL finished)
        {
          [self.navBar removeFromSuperview];
        }];
      }
    }
    break;

    case MWMPlacePageStateOpen:
    {
      static CGFloat const kOpenPlacePageOffset = 25.;

      CGFloat const h = size.height / 2. - (self.titleLabel.height + kOpenPlacePageOffset + self.typeLabel.height + self.featureTable.height + self.actionBar.height);
      _targetPoint = CGPointMake(size.width / 2., size.height + h);
      if (_targetPoint.y <= size.height / 2.)
      {
        self.navBar = [MWMPlacePageNavigationBar navigationBarWithPlacePage:self];
        [UIView animateWithDuration:.3f animations:^
        {
          self.navBar.transform = CGAffineTransformMakeTranslation(0., self.navBar.height);
        }];
      }
      break;
    }
  }
  return _targetPoint;
}

#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  CGPoint const point = [sender translationInView:self.superview];
  self.center = CGPointMake(self.center.x, self.center.y + point.y);
  [sender setTranslation:CGPointZero inView:self.superview];
  if (sender.state == UIGestureRecognizerStateEnded)
  {
    CGPoint velocity = [sender velocityInView:self.superview];
    velocity.x = 10;
    self.state = velocity.y >= 0. ? MWMPlacePageStateClosed : MWMPlacePageStateOpen;
    [self startAnimatingView:self initialVelocity:velocity];
    if (self.state == MWMPlacePageStateClosed)
    {
      self.actionBar.alpha = 0.;
      [self.placePageManager dismissPlacePage];
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
    case MWMPlacePageStatePreview:
      self.state = MWMPlacePageStateOpen;
      break;

    case MWMPlacePageStateClosed:
      self.state = MWMPlacePageStatePreview;
      break;

    case MWMPlacePageStateOpen:
      self.state = MWMPlacePageStatePreview;
      break;
  }
  [self startAnimatingView:self initialVelocity:self.springAnimation.velocity];
}

@end
