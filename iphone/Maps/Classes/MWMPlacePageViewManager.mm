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
#import "MWMPlacePage.h"
#import "MWMiPhoneLandscapePlacePage.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMiPadPlacePage.h"
#import "MWMPlacePageActionBar.h"

#include "Framework.h"

typedef NS_ENUM(NSUInteger, MWMPlacePageManagerState)
{
  MWMPlacePageManagerStateClosed,
  MWMPlacePageManagerStateOpen
};

@interface MWMPlacePageViewManager ()

@property (weak, nonatomic) UIViewController *ownerViewController;
@property (nonatomic, readwrite) MWMPlacePageEntity *entity;
@property (nonatomic) MWMPlacePage *placePage;
@property (nonatomic) MWMPlacePageManagerState state;

@end

@implementation MWMPlacePageViewManager

- (instancetype)initWithViewController:(UIViewController *)viewController
{
  self = [super init];
  if (self)
  {
    self.ownerViewController = viewController;
    self.state = MWMPlacePageManagerStateClosed;
  }
  
  return self;
}

- (void)dismissPlacePage
{
  self.state = MWMPlacePageManagerStateClosed;
  [self.placePage dismiss];
  self.placePage = nil;
}

- (void)showPlacePageWithUserMark:(std::unique_ptr<UserMarkCopy>)userMark
{
  UserMark const * mark = userMark->GetUserMark();
  self.entity = [[MWMPlacePageEntity alloc] initWithUserMark:mark];
  self.state = MWMPlacePageManagerStateOpen;
  
  if (IPAD)
    [self presentPlacePageForiPad];
  else
    [self presentPlacePageForiPhoneWithOrientation:self.ownerViewController.interfaceOrientation];
}

- (void)layoutPlacePageToOrientation:(UIInterfaceOrientation)orientation
{
  [self.placePage.extendedPlacePageView removeFromSuperview];
  [self.placePage.actionBar removeFromSuperview];
  [self presentPlacePageForiPhoneWithOrientation:orientation];
}

- (void)presentPlacePageForiPad { }

- (void)presentPlacePageForiPhoneWithOrientation:(UIInterfaceOrientation)orientation
{
  if (self.state == MWMPlacePageManagerStateClosed)
    return;

  switch (orientation)
  {
    case UIInterfaceOrientationLandscapeLeft:
    case UIInterfaceOrientationLandscapeRight:
      
      if ([self.placePage isKindOfClass:[MWMiPhoneLandscapePlacePage class]])
        [self.placePage configure];
      else
        self.placePage = [[MWMiPhoneLandscapePlacePage alloc] initWithManager:self];

      [self.placePage show];

      break;
      
    case UIInterfaceOrientationPortrait:
    case UIInterfaceOrientationPortraitUpsideDown:
      
      if ([self.placePage isKindOfClass:[MWMiPhonePortraitPlacePage class]])
        [self.placePage configure];
      else
        self.placePage = [[MWMiPhonePortraitPlacePage alloc] initWithManager:self];

      [self.placePage show];

      break;
      
    case UIInterfaceOrientationUnknown:
      
      break;
  }

}

@end
