//
//  MWMPlacePageActionBar.m
//  Maps
//
//  Created by v.mikhaylenko on 28.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageActionBar.h"
#import "MWMPlacePage.h"
#import "UIKitCategories.h"
#import "MWMBasePlacePageView.h"

static NSString * const kPlacePageActionBarNibName = @"PlacePageActionBar";

@interface MWMPlacePageActionBar ()

@property (weak, nonatomic) MWMPlacePage * placePage;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage
{
  MWMPlacePageActionBar * bar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageActionBarNibName owner:self options:nil] firstObject];
  bar.placePage = placePage;
  bar.width = placePage.extendedPlacePageView.width;
  bar.autoresizingMask = UIViewAutoresizingNone;
  return bar;
}

- (IBAction)bookmarkTap:(id)sender
{
  [self.placePage addBookmark];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event { }

@end
