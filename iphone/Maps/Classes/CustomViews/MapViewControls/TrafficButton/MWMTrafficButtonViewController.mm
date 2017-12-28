#import "MWMTrafficButtonViewController.h"
#import "MWMAlertViewController.h"
#import "MWMButton.h"
#import "MWMCommon.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMToast.h"
#import "MWMTrafficManager.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

namespace
{
CGFloat const kTopOffset = 6;

NSArray<UIImage *> * imagesWithName(NSString * name)
{
  NSUInteger const imagesCount = 3;
  NSMutableArray<UIImage *> * images = [NSMutableArray arrayWithCapacity:imagesCount];
  NSString * mode = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 1; i <= imagesCount; i += 1)
  {
    NSString * imageName = [NSString stringWithFormat:@"%@_%@_%@", name, mode, @(i).stringValue];
    [images addObject:static_cast<UIImage * _Nonnull>([UIImage imageNamed:imageName])];
  }
  return [images copy];
}
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMTrafficButtonViewController * trafficButton;

@end

@interface MWMTrafficButtonViewController ()<MWMTrafficManagerObserver>

@property(nonatomic) NSLayoutConstraint * topOffset;
@property(nonatomic) NSLayoutConstraint * leftOffset;
@property(nonatomic) CGRect availableArea;

@end

@implementation MWMTrafficButtonViewController

+ (MWMTrafficButtonViewController *)controller
{
  return [MWMMapViewControlsManager manager].trafficButton;
}

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    MapViewController * ovc = [MapViewController controller];
    [ovc addChildViewController:self];
    [ovc.view addSubview:self.view];
    [self configLayout];
    [self refreshAppearance];
    [MWMTrafficManager addObserver:self];
  }
  return self;
}

- (void)configLayout
{
  UIView * sv = self.view;
  UIView * ov = sv.superview;

  self.topOffset = [sv.topAnchor constraintEqualToAnchor:ov.topAnchor constant:kTopOffset];
  self.topOffset.active = YES;
  self.leftOffset = [sv.leadingAnchor constraintEqualToAnchor:ov.leadingAnchor
                                                     constant:kViewControlsOffsetToBounds];
  self.leftOffset.active = YES;
}

- (void)mwm_refreshUI
{
  [self.view mwm_refreshUI];
  [self refreshAppearance];
}

- (void)setHidden:(BOOL)hidden
{
  _hidden = hidden;
  [self refreshLayout];
}

- (void)refreshLayout
{
  dispatch_async(dispatch_get_main_queue(), ^{
    auto const availableArea = self.availableArea;
    auto const leftOffset =
        self.hidden ? -self.view.width : availableArea.origin.x + kViewControlsOffsetToBounds;
    [self.view.superview animateConstraintsWithAnimations:^{
      self.topOffset.constant = availableArea.origin.y + kTopOffset;
      self.leftOffset.constant = leftOffset;
    }];
  });
}

- (void)refreshAppearance
{
  MWMButton * btn = static_cast<MWMButton *>(self.view);
  UIImageView * iv = btn.imageView;

  // Traffic state machine: https://confluence.mail.ru/pages/viewpage.action?pageId=103680959
  [iv stopAnimating];
  switch ([MWMTrafficManager state])
  {
  case MWMTrafficManagerStateDisabled: btn.imageName = @"btn_traffic_off"; break;
  case MWMTrafficManagerStateEnabled: btn.imageName = @"btn_traffic_on"; break;
  case MWMTrafficManagerStateWaitingData:
    iv.animationImages = imagesWithName(@"btn_traffic_update");
    iv.animationDuration = 0.8;
    [iv startAnimating];
    break;
  case MWMTrafficManagerStateOutdated: btn.imageName = @"btn_traffic_outdated"; break;
  case MWMTrafficManagerStateNoData:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_data_unavailable")];
    break;
  case MWMTrafficManagerStateNetworkError:
    btn.imageName = @"btn_traffic_off";
    [MWMTrafficManager enableTraffic:NO];
    [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
    break;
  case MWMTrafficManagerStateExpiredData:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_update_maps_text")];
    break;
  case MWMTrafficManagerStateExpiredApp:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_update_app_message")];
    break;
  }
}

- (IBAction)buttonTouchUpInside
{
  [MWMTrafficManager enableTraffic:[MWMTrafficManager state] == MWMTrafficManagerStateDisabled];
}

+ (void)updateAvailableArea:(CGRect)frame
{
  auto controller = [self controller];
  if (CGRectEqualToRect(controller.availableArea, frame))
    return;
  controller.availableArea = frame;
  [controller refreshLayout];
}

#pragma mark - MWMTrafficManagerObserver

- (void)onTrafficStateUpdated { [self refreshAppearance]; }
@end
