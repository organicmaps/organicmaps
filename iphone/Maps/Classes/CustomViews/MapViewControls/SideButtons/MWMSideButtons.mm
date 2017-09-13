#import "MWMSideButtons.h"
#import "MWMButton.h"
#import "MWMMapViewControlsManager.h"
#import "MWMRouter.h"
#import "MWMSettings.h"
#import "MWMSideButtonsView.h"
#import "Statistics.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

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
    [images addObject:static_cast<UIImage *>([UIImage imageNamed:name])];
  }
  return images.copy;
}
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMSideButtons * sideButtons;

@end

@interface MWMSideButtons ()

@property(nonatomic) IBOutlet MWMSideButtonsView * sideView;
@property(weak, nonatomic) IBOutlet MWMButton * zoomInButton;
@property(weak, nonatomic) IBOutlet MWMButton * zoomOutButton;
@property(weak, nonatomic) IBOutlet MWMButton * locationButton;

@property(nonatomic) BOOL zoomSwipeEnabled;
@property(nonatomic, readonly) BOOL isZoomEnabled;

@property(nonatomic) MWMMyPositionMode locationMode;

@end

@implementation MWMSideButtons

+ (MWMSideButtons *)buttons { return [MWMMapViewControlsManager manager].sideButtons; }
- (instancetype)initWithParentView:(UIView *)view
{
  self = [super init];
  if (self)
  {
    [NSBundle.mainBundle loadNibNamed:kMWMSideButtonsViewNibName owner:self options:nil];
    [view addSubview:self.sideView];
    [self.sideView setNeedsLayout];
    self.zoomSwipeEnabled = NO;
    self.zoomHidden = NO;
  }
  return self;
}

+ (void)updateAvailableArea:(CGRect)frame { [[self buttons].sideView updateAvailableArea:frame]; }
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

- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode
{
  UIButton * locBtn = self.locationButton;
  [locBtn.imageView stopAnimating];

  NSArray<UIImage *> * images =
      ^NSArray<UIImage *> *(MWMMyPositionMode oldMode, MWMMyPositionMode newMode)
  {
    switch (newMode)
    {
    case MWMMyPositionModeNotFollow:
    case MWMMyPositionModeNotFollowNoPosition:
      if (oldMode == MWMMyPositionModeFollowAndRotate)
        return animationImages(@"btn_follow_and_rotate_to_get_position", 3);
      else if (oldMode == MWMMyPositionModeFollow)
        return animationImages(@"btn_follow_to_get_position", 3);
      return nil;
    case MWMMyPositionModeFollow:
      if (oldMode == MWMMyPositionModeFollowAndRotate)
        return animationImages(@"btn_follow_and_rotate_to_follow", 3);
      else if (oldMode == MWMMyPositionModeNotFollow ||
               oldMode == MWMMyPositionModeNotFollowNoPosition)
        return animationImages(@"btn_get_position_to_follow", 3);
      return nil;
    case MWMMyPositionModePendingPosition: return nil;
    case MWMMyPositionModeFollowAndRotate:
      if (oldMode == MWMMyPositionModeFollow)
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

- (void)refreshLocationButtonState:(MWMMyPositionMode)state
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.locationButton.imageView.isAnimating)
    {
      [self refreshLocationButtonState:state];
    }
    else
    {
      MWMButton * locBtn = self.locationButton;
      switch (state)
      {
      case MWMMyPositionModePendingPosition:
      {
        NSArray<UIImage *> * images = animationImages(@"btn_pending", 12);
        locBtn.imageView.animationDuration = 1.2;
        locBtn.imageView.animationImages = images;
        locBtn.imageView.animationRepeatCount = 0;
        locBtn.imageView.image = images.lastObject;
        [locBtn.imageView startAnimating];
        break;
      }
      case MWMMyPositionModeNotFollow:
      case MWMMyPositionModeNotFollowNoPosition: locBtn.imageName = @"btn_get_position"; break;
      case MWMMyPositionModeFollow: locBtn.imageName = @"btn_follow"; break;
      case MWMMyPositionModeFollowAndRotate: locBtn.imageName = @"btn_follow_and_rotate"; break;
      }
    }
  });
}

#pragma mark - Actions

- (IBAction)zoomTouchDown:(UIButton *)sender { self.zoomSwipeEnabled = YES; }
- (IBAction)zoomTouchUpInside:(UIButton *)sender
{
  self.zoomSwipeEnabled = NO;
  if ([sender isEqual:self.zoomInButton])
    [self zoomIn];
  else
    [self zoomOut];
}

- (IBAction)zoomTouchUpOutside:(UIButton *)sender { self.zoomSwipeEnabled = NO; }
- (IBAction)zoomSwipe:(UIPanGestureRecognizer *)sender
{
  if (!self.zoomSwipeEnabled)
    return;
  UIView * const superview = self.sideView.superview;
  CGFloat const translation =
      -[sender translationInView:superview].y / superview.bounds.size.height;

  CGFloat const scaleFactor = exp(translation);
  GetFramework().Scale(scaleFactor, false);
}

- (IBAction)locationTouchUpInside
{
  [Statistics logEvent:kStatMenu withParameters:@{kStatButton : kStatLocation}];
  GetFramework().SwitchMyPositionNextMode();
}

#pragma mark - Properties

- (BOOL)zoomHidden { return self.sideView.zoomHidden; }
- (void)setZoomHidden:(BOOL)zoomHidden
{
  if ([MWMRouter isRoutingActive])
    self.sideView.zoomHidden = NO;
  else
    self.sideView.zoomHidden = [MWMSettings zoomButtonsEnabled] ? zoomHidden : YES;
}

- (BOOL)hidden { return self.sideView.hidden; }
- (void)setHidden:(BOOL)hidden { [self.sideView setHidden:hidden animated:YES]; }
@end
