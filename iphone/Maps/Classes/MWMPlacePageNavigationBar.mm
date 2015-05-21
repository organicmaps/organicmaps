//
//  MWMPlacePageNavigationBar.m
//  Maps
//
//  Created by v.mikhaylenko on 13.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageNavigationBar.h"
#import "MWMiPhonePortraitPlacePage.h"
#import "MWMPlacePageViewManager.h"
#import "MWMBasePlacePageView.h"
#import "UIKitCategories.h"

static NSString * const kPlacePageNavigationBarNibName = @"PlacePageNavigationBar";

@interface MWMPlacePageNavigationBar ()

@property (weak, nonatomic, readwrite) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) MWMiPhonePortraitPlacePage * placePage;

@end

@implementation MWMPlacePageNavigationBar

+ (instancetype)navigationBarWithPlacePage:(MWMiPhonePortraitPlacePage *)placePage
{
  MWMPlacePageNavigationBar * navBar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageNavigationBarNibName owner:nil options:nil] firstObject];
  navBar.placePage = placePage;
  navBar.width = placePage.extendedPlacePageView.bounds.size.width;
  navBar.titleLabel.text = placePage.basePlacePageView.titleLabel.text;
  navBar.center = CGPointMake(placePage.extendedPlacePageView.superview.origin.x + navBar.width / 2., placePage.extendedPlacePageView.superview.origin.y - navBar.height / 2.);
  [placePage.extendedPlacePageView.superview addSubview:navBar];
  return navBar;
}

- (IBAction)backTap:(id)sender
{
  [self.placePage.manager dismissPlacePage];
  [UIView animateWithDuration:.25f animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., - self.height);
  }
  completion:^(BOOL finished)
  {
    [self removeFromSuperview];
  }];
}

@end
