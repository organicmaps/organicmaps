//
//  MWMPlacePageViewManager.m
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageViewManager.h"
#import "MWMBasePlacePageView.h"
#import "UIKitCategories.h"
#import "MWMPlacePageEntity.h"

#include "Framework.h"

@interface MWMPlacePageViewManager ()

@property (weak, nonatomic) UIViewController *ownerViewController;
@property (nonatomic) MWMPlacePageEntity *entity;
@property (nonatomic) MWMBasePlacePageView *placePageView;

@end

@implementation MWMPlacePageViewManager

- (instancetype)initWithViewController:(UIViewController *)viewController
{
  self = [super init];
  if (self)
    self.ownerViewController = viewController;

  return self;
}

- (void)dismissPlacePage
{
  [self.placePageView dismiss];
  self.placePageView = nil;
}

- (void)showPlacePageWithUserMark:(std::unique_ptr<UserMarkCopy>)userMark
{
  UserMark const * mark = userMark->GetUserMark();
  self.entity = [[MWMPlacePageEntity alloc] initWithUserMark:mark];

  /*
   1. Determine if this iPad or iPhone
   2. Get current place page (check, if current place page already exist)
      2.1 If yes - configure with existed entity
      2.2 If no - create
   3. Show
   */

  if (self.placePageView == nil)
  {
    self.placePageView = [[MWMBasePlacePageView alloc] init];
    [self.placePageView configureWithEntity:self.entity];
    self.placePageView.placePageManager = self;
    [self.placePageView show];
  }
  else
  {
    [self.placePageView configureWithEntity:self.entity];
  }

  //TODO (Vlad): Implement setState: method.
  if (IPAD)
  {

  }
  else
  {
    [self.placePageView didTap:nil];
  }
}

- (void)layoutPlacePage
{
  [self dismissPlacePage];
}

- (MWMBasePlacePageView *)presentPlacePageForiPhone
{
  MWMBasePlacePageView * placePage;
  switch (self.ownerViewController.interfaceOrientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:

      break;

    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:

      break;

    case UIInterfaceOrientationUnknown:

      break;
  }
  return placePage;
}

@end
