//
//  MWMPlacePageNavigationBar.m
//  Maps
//
//  Created by v.mikhaylenko on 13.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageNavigationBar.h"
#import "MWMiPhonePortraitPlacePageView.h"
#import "MWMPlacePageViewManager.h"
#import "UIKitCategories.h"

static NSString * const kPlacePageNavigationBarNibName = @"PlacePageNavigationBar";

@interface MWMPlacePageNavigationBar ()

@property (weak, nonatomic, readwrite) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) MWMiPhonePortraitPlacePageView *placePage;

@end

@implementation MWMPlacePageNavigationBar

+ (instancetype)navigationBarWithPlacePage:(MWMiPhonePortraitPlacePageView *)placePage
{
  MWMPlacePageNavigationBar *navBar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageNavigationBarNibName owner:nil options:nil] firstObject];
  navBar.placePage = placePage;
  navBar.width = placePage.bounds.size.width;
  navBar.titleLabel.text = placePage.titleLabel.text;
  navBar.center = CGPointMake(placePage.superview.origin.x + navBar.width / 2., placePage.superview.origin.y - navBar.height / 2.);
  [placePage.superview addSubview:navBar];
  return navBar;
}

- (IBAction)backTap:(id)sender
{
  [self.placePage.placePageManager dismissPlacePage];
  [UIView animateWithDuration:.3 animations:^
  {
    self.transform = CGAffineTransformMakeTranslation(0., -self.height);
  }
  completion:^(BOOL finished)
  {
    [self removeFromSuperview];
  }];
}

@end
