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

@property (weak, nonatomic) IBOutlet UIButton * bookmarkButton;
@property (weak, nonatomic) IBOutlet UIButton * shareButton;
@property (weak, nonatomic) IBOutlet UIButton * routeButton;
@property (weak, nonatomic) IBOutlet UILabel * routeLabel;
@property (weak, nonatomic) IBOutlet UILabel * bookmarkLabel;
@property (weak, nonatomic) IBOutlet UILabel * shareLabel;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage
{
  MWMPlacePageActionBar * bar = [[[NSBundle mainBundle] loadNibNamed:kPlacePageActionBarNibName owner:self options:nil] firstObject];
  bar.placePage = placePage;
  BOOL const isMyPosition = placePage.manager.entity.type == MWMPlacePageEntityTypeMyPosition;
  bar.routeButton.hidden = isMyPosition;
  bar.autoresizingMask = UIViewAutoresizingNone;
  [self setupBookmarkButton:bar];
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
  self.isBookmark = !self.isBookmark;
  NSMutableString * eventName = @"ppBookmarkButtonTap".mutableCopy;
  if (self.isBookmark)
  {
    [sender.imageView startAnimating];
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
  BOOL const isMyPosition = self.placePage.manager.entity.type == MWMPlacePageEntityTypeMyPosition;
  CGPoint const center = self.center;
  CGFloat const leftOffset = 18.;
  if (isMyPosition)
  {
    CGSize const size = [[UIScreen mainScreen] bounds].size;
    CGFloat const maximumWidth = 360.;
    CGFloat const actualWidth = MIN(MIN(size.height, size.width), maximumWidth);
    self.bookmarkButton.center = CGPointMake(3. * actualWidth / 4., self.bookmarkButton.center.y);
    self.shareButton.center = CGPointMake(actualWidth / 4., self.bookmarkButton.center.y);
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
}

#pragma mark - Properties

- (void)setIsBookmark:(BOOL)isBookmark
{
  _isBookmark = isBookmark;
  self.bookmarkButton.selected = isBookmark;
  self.bookmarkLabel.text = L(isBookmark ? @"delete" : @"save");
}

@end
