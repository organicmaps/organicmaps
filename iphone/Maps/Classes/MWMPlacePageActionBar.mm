#import "Common.h"
#import "MapsAppDelegate.h"
#import "MWMBasePlacePageView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"

#include "Framework.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

extern NSString * const kAlohalyticsTapEventKey;
static NSString * const kPlacePageActionBarNibName = @"PlacePageActionBar";

@interface MWMPlacePageActionBar ()

@property (weak, nonatomic) MWMPlacePage * placePage;
@property (weak, nonatomic) IBOutlet UIButton * apiBackButton;
@property (weak, nonatomic) IBOutlet UIButton * bookmarkButton;
@property (weak, nonatomic) IBOutlet UIButton * routeButton;
@property (weak, nonatomic) IBOutlet UILabel * apiBackLabel;
@property (weak, nonatomic) IBOutlet UILabel * routeLabel;
@property (weak, nonatomic) IBOutlet UILabel * bookmarkLabel;
@property (weak, nonatomic) IBOutlet UILabel * shareLabel;

@end

@implementation MWMPlacePageActionBar

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage
{
  BOOL const isPrepareRouteMode = MapsAppDelegate.theApp.routingPlaneMode != MWMRoutingPlaneModeNone;
  NSUInteger const i = isPrepareRouteMode ? 1 : 0;
  MWMPlacePageActionBar * bar = [NSBundle.mainBundle
                                 loadNibNamed:kPlacePageActionBarNibName owner:nil options:nil][i];
  NSAssert(i == bar.tag, @"Incorrect view!");
  bar.isPrepareRouteMode = isPrepareRouteMode;
  bar.placePage = placePage;
  if (isPrepareRouteMode)
    return bar;

  [bar setupBookmarkButton];
  [bar configureWithPlacePage:placePage];
  return bar;
}

- (void)configureWithPlacePage:(MWMPlacePage *)placePage
{
  self.placePage = placePage;
  switch (placePage.manager.entity.type)
  {
    case MWMPlacePageEntityTypeAPI:
      [self setupApiBar];
      break;
    case MWMPlacePageEntityTypeBookmark:
      [self setupBookmarkBar];
      break;
    case MWMPlacePageEntityTypeMyPosition:
      [self setupMyPositionBar];
      break;
    default:
      [self setupDefaultBar];
      break;
  }
  self.autoresizingMask = UIViewAutoresizingNone;
  [self setNeedsLayout];
}

- (void)setupApiBar
{
  // Four buttons: Back, Share, Bookmark and Route
  self.isBookmark = NO;
  [self allButtons];
}

- (void)setupMyPositionBar
{
  // Two buttons: Share and Bookmark
  self.isBookmark = NO;
  [self twoButtons];
}

- (void)setupBookmarkBar
{
  // Three buttons: Share, Bookmark and Route
  self.isBookmark = YES;
  [self threeButtons];
}

- (void)setupDefaultBar
{
  // Three buttons: Share, Bookmark and Route
  self.isBookmark = NO;
  [self threeButtons];
}

- (void)threeButtons
{
  self.routeButton.hidden = NO;
  self.routeLabel.hidden = NO;
  self.apiBackButton.hidden = YES;
  self.apiBackLabel.hidden = YES;
}

- (void)twoButtons
{
  self.routeButton.hidden = YES;
  self.routeLabel.hidden = YES;
  self.apiBackButton.hidden = YES;
  self.apiBackLabel.hidden = YES;
}

- (void)allButtons
{
  for (UIView * v in self.subviews)
    v.hidden = NO;
}

- (void)setupBookmarkButton
{
  UIButton * btn = self.bookmarkButton;
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateHighlighted|UIControlStateSelected];

  NSUInteger const animationImagesCount = 11;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"ic_bookmarks_%@", @(i+1)]];

  UIImageView * animationIV = btn.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationRepeatCount = 1;
}

- (IBAction)fromTap
{
  [self.placePage.manager routeFrom];
}

- (IBAction)toTap
{
  [self.placePage.manager routeTo];
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

- (void)layoutSubviews
{
  CGFloat const buttonWidth = 80.;
  CGSize const size = UIScreen.mainScreen.bounds.size;
  CGFloat const maximumWidth = 360.;
  CGFloat const screenWidth = MIN(size.height, size.width);
  CGFloat const actualWidth = IPAD ? maximumWidth : (size.height < size.width ? MIN(screenWidth, maximumWidth) : screenWidth);
  switch (self.placePage.manager.entity.type)
  {
    case MWMPlacePageEntityTypeAPI:
    {
      CGFloat const boxWidth = 4 * buttonWidth;
      CGFloat const leftOffset = (actualWidth - boxWidth) / 5.;
      self.apiBackButton.minX = leftOffset;
      self.shareButton.minX = self.apiBackButton.maxX + leftOffset;
      self.bookmarkButton.minX = self.shareButton.maxX + leftOffset;
      self.routeButton.minX = self.bookmarkButton.maxX;
      break;
    }
    case MWMPlacePageEntityTypeMyPosition:
    {
      CGFloat const boxWidth = 2 * buttonWidth;
      CGFloat const leftOffset = (actualWidth - boxWidth) / 3.;
      self.shareButton.minX = leftOffset;
      self.bookmarkButton.minX = actualWidth - leftOffset - buttonWidth;
      break;
    }
    default:
    {
      CGFloat const boxWidth = 3 * buttonWidth;
      CGFloat const leftOffset = (actualWidth - boxWidth) / 4.;
      self.shareButton.minX = leftOffset;
      self.bookmarkButton.minX = self.shareButton.maxX + leftOffset;
      self.routeButton.minX = self.bookmarkButton.maxX + leftOffset;
      break;
    }
  }
  self.apiBackLabel.minX = self.apiBackButton.minX;
  self.shareLabel.minX = self.shareButton.minX;
  self.bookmarkLabel.minX = self.bookmarkButton.minX;
  self.routeLabel.minX = self.routeButton.minX;
}

- (IBAction)shareTap
{
  [self.placePage share];
}

- (IBAction)routeTap
{
  [self.placePage route];
}

- (IBAction)backTap
{
  [self.placePage apiBack];
}

#pragma mark - Properties

- (void)setIsBookmark:(BOOL)isBookmark
{
  _isBookmark = isBookmark;
  self.bookmarkButton.selected = isBookmark;
  self.bookmarkLabel.text = L(isBookmark ? @"delete" : @"save");
}

@end
