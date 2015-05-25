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
#import "MWMPlacePageEntity.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"

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
@property (nonatomic) CGFloat keyboardHeight;

@end

@implementation MWMiPhonePortraitPlacePage

- (void)configure
{
  [super configure];
  UIView const * view = self.manager.ownerViewController.view;
  if ([view.subviews containsObject:self.extendedPlacePageView])
    return;

  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const width = size.width > size.height ? size.height : size.width;
  CGFloat const height = size.width > size.height ? size.width : size.height;
  self.extendedPlacePageView.frame = CGRectMake(0., height, width, 2 * height);
  [view addSubview:self.extendedPlacePageView];
  self.actionBar.width = width;
  self.actionBar.center = CGPointMake(width / 2., height + self.actionBar.height / 2.);
  [view addSubview:self.actionBar];

  [UIView animateWithDuration:0.3f delay:0. options:UIViewAnimationOptionCurveEaseOut animations:^
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
  self.state = MWMiPhonePortraitPlacePageStateClosed;
  [MWMPlacePageNavigationBar remove];
  self.keyboardHeight = 0.;
  [super dismiss];
}

- (void)addBookmark
{
  [super addBookmark];
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)removeBookmark
{
 [super removeBookmark];
  self.keyboardHeight = 0.;
  self.state = MWMiPhonePortraitPlacePageStatePreview;
}

- (void)setState:(MWMiPhonePortraitPlacePageState)state
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  BOOL const isLandscape = size.width > size.height;
  CGFloat const width = isLandscape ? size.height : size.width;
  CGFloat const height = isLandscape ? size.width : size.height;
  static CGFloat const kPlacePageBottomOffset = 30.;
  switch (state)
  {
    case MWMiPhonePortraitPlacePageStateClosed:
//      GetFramework().GetBalloonManager().RemovePin();
      self.targetPoint = CGPointMake(self.extendedPlacePageView.width / 2., self.extendedPlacePageView.height * 2.);
      break;

    case MWMiPhonePortraitPlacePageStatePreview:
    {
      CGFloat const typeHeight = self.basePlacePageView.typeLabel.text.length > 0 ? self.basePlacePageView.typeLabel.height : self.basePlacePageView.typeDescriptionView.height;
      CGFloat const h = height - (self.basePlacePageView.titleLabel.height + kPlacePageBottomOffset + typeHeight + self.actionBar.height + 1);
      self.targetPoint = CGPointMake(width / 2., height + h);

      [MWMPlacePageNavigationBar dismissNavigationBar];
      break;
    }

    case MWMiPhonePortraitPlacePageStateOpen:
    {
      CGFloat const typeHeight = self.basePlacePageView.typeLabel.text.length > 0 ? self.basePlacePageView.typeLabel.height : self.basePlacePageView.typeDescriptionView.height;
      CGFloat const h = height - (self.basePlacePageView.titleLabel.height + kPlacePageBottomOffset + typeHeight + [(UITableView *)self.basePlacePageView.featureTable height] + self.actionBar.height + 1 + self.keyboardHeight);
      self.targetPoint = CGPointMake(width / 2., height + h);

      if (self.targetPoint.y <= height)
        [MWMPlacePageNavigationBar showNavigationBarForPlacePage:self];
      else
        [MWMPlacePageNavigationBar dismissNavigationBar];

      break;
    }
  }
  _state = state;
  [self startAnimatingPlacePage:self initialVelocity:self.springAnimation.velocity];

  NSString * anchorImageName;
  NSNumber const * widthNumber = @((NSUInteger)width);
  if (_state == MWMiPhonePortraitPlacePageStateOpen)
    anchorImageName = [@"bg_placepage_tablet_open_" stringByAppendingString:widthNumber.stringValue];
  else
    anchorImageName = [@"bg_placepage_tablet_normal_" stringByAppendingString:widthNumber.stringValue];

  self.anchorImageView.image = [UIImage imageNamed:anchorImageName];

}

#pragma mark - Actions

- (IBAction)didPan:(UIPanGestureRecognizer *)sender
{
  CGPoint const point = [sender translationInView:self.extendedPlacePageView.superview];
  self.extendedPlacePageView.center = CGPointMake(self.extendedPlacePageView.center.x, self.extendedPlacePageView.center.y + point.y);
  CGSize const size = UIScreen.mainScreen.bounds.size;
  BOOL const isLandscape = size.width > size.height;
  CGFloat const height = isLandscape ? size.width : size.height;

  if (self.extendedPlacePageView.center.y <= height)
    [MWMPlacePageNavigationBar showNavigationBarForPlacePage:self];
  else
    [MWMPlacePageNavigationBar dismissNavigationBar];

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
  [super didTap:sender];
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

- (void)willStartEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  self.keyboardHeight = keyboardHeight;
  self.state = MWMiPhonePortraitPlacePageStateOpen;
}

- (void)willFinishEditingBookmarkTitle:(CGFloat)keyboardHeight
{
  self.keyboardHeight = 0.;
  //Just setup current state.
  MWMiPhonePortraitPlacePageState currentState = self.state;
  self.state = currentState;
}

@end
