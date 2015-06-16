//
//  MWMPlacePageActionBar.m
//  Maps
//
//  Created by v.mikhaylenko on 28.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageActionBar.h"
#import "MWMPlacePage.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePageEntity.h"
#import "UIKitCategories.h"

#include "Framework.h"

#import "../../3party/Alohalytics/src/alohalytics_objc.h"

extern NSString * const kAlohalyticsTapEventKey;
static NSString * const kPlacePageActionBarNibName = @"PlacePageActionBar";

@interface MWMPlacePageActionBar ()

@property (weak, nonatomic) MWMPlacePage * placePage;

@property (weak, nonatomic) IBOutlet UIButton * shareButton;
@property (weak, nonatomic) IBOutlet UILabel * routeLabel;
@property (weak, nonatomic) IBOutlet UILabel * bookmarkLabel;
@property (weak, nonatomic) IBOutlet UILabel * shareLabel;
@property (nonatomic) UIActivityIndicatorView * indicatior;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage
{
  MWMPlacePageActionBar * bar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageActionBarNibName owner:self options:nil] firstObject];
  bar.placePage = placePage;
  BOOL const isMyPosition = placePage.manager.entity.type == MWMPlacePageEntityTypeMyPosition;
  bar.routeButton.hidden = isMyPosition;
  bar.autoresizingMask = UIViewAutoresizingNone;
  return bar;
}

- (IBAction)bookmarkTap:(UIButton *)sender
{
  sender.selected = !sender.selected;
  NSMutableString *eventName = @"ppBookmarkButtonTap".mutableCopy;
  if (sender.selected)
  {
    [self.placePage addBookmark];
    [eventName appendString:@"Add"];
  }
  else
  {
    [self.placePage removeBookmark];
    [eventName appendString:@"Delete"];
  }
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:eventName];
}

- (void)configureForMyPosition:(BOOL)isMyPosition
{
  self.routeLabel.hidden = isMyPosition;
  self.routeButton.hidden = isMyPosition;
  [self layoutSubviews];
}

- (void)layoutSubviews
{
  BOOL isMyPosition = self.placePage.manager.entity.type == MWMPlacePageEntityTypeMyPosition;

  if (GetFramework().IsRouteBuilding())
    [self startActivityIndicator];

  CGPoint const center = self.center;
  CGFloat const leftOffset = 18.;

  if (isMyPosition)
  {
    CGSize const size = [[UIScreen mainScreen] bounds].size;
    CGFloat const maximumWidth = 360.;
    CGFloat const screenWidth = size.width > size.height ? (size.height > maximumWidth ? maximumWidth : size.height) : size.width;

    self.bookmarkButton.center = CGPointMake(3. * screenWidth / 4., self.bookmarkButton.center.y);
    self.shareButton.center = CGPointMake(screenWidth / 4., self.bookmarkButton.center.y);
  }
  else
  {
    self.bookmarkButton.center = CGPointMake(center.x, self.bookmarkButton.center.y);
    self.shareButton.center = CGPointMake(leftOffset + self.shareButton.width / 2., self.shareButton.center.y);
  }

  self.shareLabel.center = CGPointMake(self.shareButton.center.x, self.shareLabel.center.y);
  self.bookmarkLabel.center = CGPointMake(self.bookmarkButton.center.x, self.bookmarkLabel.center.y);
}

- (IBAction)shareTap
{
  [self.placePage share];
}

- (IBAction)routeTap
{
  [self.placePage route];
  [self startActivityIndicator];
}

- (void)startActivityIndicator
{
  [self.indicatior removeFromSuperview];
  self.indicatior = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
  self.indicatior.center = self.routeButton.center;
  self.routeButton.hidden = YES;
  [self.indicatior startAnimating];
  [self addSubview:self.indicatior];
}

- (void)dismissActivityIndicatior
{
  [self.indicatior removeFromSuperview];
  self.indicatior = nil;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
// Prevent super call to stop event propagation
// [super touchesBegan:touches withEvent:event];
}

@end
