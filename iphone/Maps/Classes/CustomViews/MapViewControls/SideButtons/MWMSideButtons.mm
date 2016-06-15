#import "Common.h"
#import "MWMButton.h"
#import "MWMSideButtons.h"
#import "MWMSideButtonsView.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"
#include "platform/settings.hpp"
#include "indexer/scales.hpp"

extern NSString * const kAlohalyticsTapEventKey;

namespace
{
NSString * const kMWMSideButtonsViewNibName = @"MWMSideButtonsView";

NSArray<UIImage *> * animationImages(NSString * animationTemplate, NSUInteger imagesCount)
{
  NSMutableArray<UIImage *> * images = [NSMutableArray arrayWithCapacity:imagesCount];
  NSString * mode = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 1; i <= imagesCount; i += 1)
  {
    NSString * name =
        [NSString stringWithFormat:@"%@_%@_%@", animationTemplate, mode, @(i).stringValue];
    [images addObject:[UIImage imageNamed:name]];
  }
  return images.copy;
}
}  // namespace

@interface MWMSideButtons()

@property (nonatomic) IBOutlet MWMSideButtonsView * sideView;
@property (weak, nonatomic) IBOutlet MWMButton * zoomInButton;
@property (weak, nonatomic) IBOutlet MWMButton * zoomOutButton;
@property (weak, nonatomic) IBOutlet MWMButton * locationButton;

@property (nonatomic) BOOL zoomSwipeEnabled;
@property (nonatomic, readonly) BOOL isZoomEnabled;

@property (nonatomic) location::EMyPositionMode locationMode;

@end

@implementation MWMSideButtons

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kMWMSideButtonsViewNibName owner:self options:nil];
    [view addSubview:self.sideView];
    [self.sideView layoutIfNeeded];
    self.sideView.topBound = 0.0;
    self.sideView.bottomBound = view.height;
    self.zoomSwipeEnabled = NO;
  }
  return self;
}

- (void)setTopBound:(CGFloat)bound
{
  self.sideView.topBound = bound;
}

- (void)setBottomBound:(CGFloat)bound
{
  self.sideView.bottomBound = bound;
}

- (void)zoomIn
{
  [Statistics logEvent:kStatEventName(kStatZoom, kStatIn)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"+"];
  GetFramework().Scale(Framework::SCALE_MAG, true);
}

- (void)zoomOut
{
  [Statistics logEvent:kStatEventName(kStatZoom, kStatOut)];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"-"];
  GetFramework().Scale(Framework::SCALE_MIN, true);
}

- (void)mwm_refreshUI
{
  [self.sideView mwm_refreshUI];
  [self.locationButton.imageView stopAnimating];
  [self refreshLocationButtonState:self.locationMode];
}

- (void)processMyPositionStateModeEvent:(location::EMyPositionMode)mode
{
  UIButton * locBtn = self.locationButton;
  [locBtn.imageView stopAnimating];

  NSArray<UIImage *> * images =
      ^NSArray<UIImage *> *(location::EMyPositionMode oldMode, location::EMyPositionMode newMode)
  {
    switch (newMode)
    {
    case location::NotFollow:
    case location::NotFollowNoPosition:
      if (oldMode == location::FollowAndRotate)
        return animationImages(@"btn_follow_and_rotate_to_get_position", 3);
      else if (oldMode == location::Follow)
        return animationImages(@"btn_follow_to_get_position", 3);
      return nil;
    case location::Follow:
      if (oldMode == location::FollowAndRotate)
        return animationImages(@"btn_follow_and_rotate_to_follow", 3);
      else if (oldMode == location::NotFollow || oldMode == location::NotFollowNoPosition)
        return animationImages(@"btn_get_position_to_follow", 3);
      return nil;
    case location::PendingPosition: return nil;
    case location::FollowAndRotate:
      if (oldMode == location::Follow)
        return animationImages(@"btn_follow_to_follow_and_rotate", 3);
      return nil;
    }
  }
  (self.locationMode, mode);
  locBtn.imageView.animationImages = images;
  if (images)
  {
    locBtn.imageView.animationDuration = 0.0;
    locBtn.imageView.animationRepeatCount = 1;
    locBtn.imageView.image = images.lastObject;
    [locBtn.imageView startAnimating];
  }
  [self refreshLocationButtonState:mode];
  self.locationMode = mode;
}

#pragma mark - Location button

- (void)refreshLocationButtonState:(location::EMyPositionMode)state
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    if (self.locationButton.imageView.isAnimating)
    {
      [self refreshLocationButtonState:state];
    }
    else
    {
      MWMButton * locBtn = self.locationButton;
      switch (state)
      {
        case location::PendingPosition:
        {
          NSArray<UIImage *> * images = animationImages(@"btn_pending", 12);
          locBtn.imageView.animationDuration = 0.8;
          locBtn.imageView.animationImages = images;
          locBtn.imageView.animationRepeatCount = 0;
          locBtn.imageView.image = images.lastObject;
          [locBtn.imageView startAnimating];
          break;
        }
        case location::NotFollow:
        case location::NotFollowNoPosition:
          locBtn.imageName = @"btn_get_position";
          break;
        case location::Follow:
          locBtn.imageName = @"btn_follow";
          break;
        case location::FollowAndRotate:
          locBtn.imageName = @"btn_follow_and_rotate";
          break;
      }
    }
  });
}

#pragma mark - Actions

- (IBAction)zoomTouchDown:(UIButton *)sender
{
  self.zoomSwipeEnabled = YES;
}

- (IBAction)zoomTouchUpInside:(UIButton *)sender
{
  self.zoomSwipeEnabled = NO;
  if ([sender isEqual:self.zoomInButton])
    [self zoomIn];
  else
    [self zoomOut];
}

- (IBAction)zoomTouchUpOutside:(UIButton *)sender
{
  self.zoomSwipeEnabled = NO;
}

- (IBAction)zoomSwipe:(UIPanGestureRecognizer *)sender
{
  if (!self.zoomSwipeEnabled)
    return;
  UIView * const superview = self.sideView.superview;
  CGFloat const translation = -[sender translationInView:superview].y / superview.bounds.size.height;

  CGFloat const scaleFactor = exp(translation);
  GetFramework().Scale(scaleFactor, false);
}

- (IBAction)locationTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatLocation}];
  GetFramework().SwitchMyPositionNextMode();
}

#pragma mark - Properties

- (BOOL)isZoomEnabled
{
  bool zoomButtonsEnabled = true;
  (void)settings::Get("ZoomButtonsEnabled", zoomButtonsEnabled);
  return zoomButtonsEnabled;
}

- (BOOL)zoomHidden
{
  return self.sideView.zoomHidden;
}

- (void)setZoomHidden:(BOOL)zoomHidden
{
  self.sideView.zoomHidden = [self isZoomEnabled] ? zoomHidden : YES;
}

- (BOOL)hidden
{
  return self.sideView.hidden;
}

- (void)setHidden:(BOOL)hidden
{
  [self.sideView setHidden:hidden animated:YES];
}

@end
