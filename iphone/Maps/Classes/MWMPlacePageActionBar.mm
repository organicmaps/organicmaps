//
//  MWMPlacePageActionBar.m
//  Maps
//
//  Created by v.mikhaylenko on 28.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MapsAppDelegate.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "UIKitCategories.h"

#include "Framework.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

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
  BOOL const isPedestrian = GetFramework().GetRouter() == routing::RouterType::Pedestrian;
  NSString * routeImageName = isPedestrian ? @"ic_route_walk" : @"ic_route";
  [bar.routeButton setImage:[UIImage imageNamed:routeImageName] forState:UIControlStateNormal];
  [self setupBookmarkButton:(MWMPlacePageActionBar *)bar];
  return bar;
}

+ (void)setupBookmarkButton:(MWMPlacePageActionBar *)bar
{
  UIButton * btn = bar.bookmarkButton;
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateHighlighted|UIControlStateSelected];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_off_pressed"] forState:UIControlStateHighlighted];

  NSUInteger const animationImagesCount = 11;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"ic_bookmarks_%@", @(i+1)]];

  UIImageView * animationIV = btn.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationRepeatCount = 1;
}

- (IBAction)bookmarkTap:(UIButton *)sender
{
  sender.selected = !sender.selected;
  NSMutableString * eventName = @"ppBookmarkButtonTap".mutableCopy;
  if (sender.selected)
  {
    [sender.imageView startAnimating];
    self.bookmarkLabel.text = L(@"delete");
    [self.placePage addBookmark];
    [eventName appendString:@"Add"];
  }
  else
  {

    self.bookmarkLabel.text = L(@"save");
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
  BOOL const isMyPosition = self.placePage.manager.entity.type == MWMPlacePageEntityTypeMyPosition;
  if (GetFramework().IsRouteBuilding() && !isMyPosition)
    [self startActivityIndicator];

  CGPoint const center = self.center;
  CGFloat const leftOffset = 18.;
  if (isMyPosition)
  {
    CGSize const size = [[UIScreen mainScreen] bounds].size;
    CGFloat const maximumWidth = 360.;
    CGFloat const actualWidth = MIN(MIN(size.height, size.width), maximumWidth);
    self.bookmarkButton.center = CGPointMake(3. * actualWidth / 4., self.bookmarkButton.center.y);
    self.shareButton.center = CGPointMake(actualWidth / 4., self.bookmarkButton.center.y);
    [self.indicatior removeFromSuperview];
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
  [self startActivityIndicator];
  [self.placePage route];
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
  self.routeButton.hidden = NO;
}

@end
