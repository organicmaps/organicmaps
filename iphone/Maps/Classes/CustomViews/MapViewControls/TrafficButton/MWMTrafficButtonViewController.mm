#import "MWMTrafficButtonViewController.h"
#import "MWMCommon.h"
#import "MWMAlertViewController.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMToast.h"
#import "MWMTrafficManager.h"
#import "MapViewController.h"

namespace
{
CGFloat const kTopOffset = 26;
CGFloat const kTopShiftedOffset = 6;

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

  self.topOffset = [NSLayoutConstraint constraintWithItem:sv
                                                attribute:NSLayoutAttributeTop
                                                relatedBy:NSLayoutRelationEqual
                                                   toItem:ov
                                                attribute:NSLayoutAttributeTop
                                               multiplier:1
                                                 constant:kTopOffset];
  self.leftOffset = [NSLayoutConstraint constraintWithItem:sv
                                                 attribute:NSLayoutAttributeLeading
                                                 relatedBy:NSLayoutRelationEqual
                                                    toItem:ov
                                                 attribute:NSLayoutAttributeLeading
                                                multiplier:1
                                                  constant:kViewControlsOffsetToBounds];

  [ov addConstraints:@[ self.topOffset, self.leftOffset ]];
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

- (void)setTopBound:(CGFloat)topBound
{
  if (_topBound == topBound)
    return;
  _topBound = topBound;
  [self refreshLayout];
}

- (void)setLeftBound:(CGFloat)leftBound
{
  if (_leftBound == leftBound)
    return;
  _leftBound = leftBound;
  [self refreshLayout];
}

- (void)refreshLayout
{
  runAsyncOnMainQueue(^{
    CGFloat const topOffset = self.topBound > 0 ? self.topBound + kTopShiftedOffset : kTopOffset;
    CGFloat const leftOffset =
        self.hidden ? -self.view.width : self.leftBound + kViewControlsOffsetToBounds;
    UIView * ov = self.view.superview;
    [ov layoutIfNeeded];
    self.topOffset.constant = topOffset;
    self.leftOffset.constant = leftOffset;
    [UIView animateWithDuration:kDefaultAnimationDuration
                     animations:^{
                       [ov layoutIfNeeded];
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
  case TrafficManager::TrafficState::Disabled:
    btn.imageName = @"btn_traffic_off";
    break;
  case TrafficManager::TrafficState::Enabled:
    btn.imageName = @"btn_traffic_on";
    break;
  case TrafficManager::TrafficState::WaitingData:
    iv.animationImages = imagesWithName(@"btn_traffic_update");
    iv.animationDuration = 0.8;
    [iv startAnimating];
    break;
  case TrafficManager::TrafficState::Outdated:
    btn.imageName = @"btn_traffic_outdated";
    break;
  case TrafficManager::TrafficState::NoData:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_data_unavailable")];
    break;
  case TrafficManager::TrafficState::NetworkError:
    btn.imageName = @"btn_traffic_off";
    [MWMTrafficManager enableTraffic:NO];
    [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
    break;
  case TrafficManager::TrafficState::ExpiredApp:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_update_app_message")];
    break;
  case TrafficManager::TrafficState::ExpiredData:
    btn.imageName = @"btn_traffic_on";
    [MWMToast showWithText:L(@"traffic_update_maps_text")];
    break;
  }
}
- (IBAction)buttonTouchUpInside
{
  if ([MWMTrafficManager state] == TrafficManager::TrafficState::Disabled)
    [MWMTrafficManager enableTraffic:YES];
  else
    [MWMTrafficManager enableTraffic:NO];
}

#pragma mark - MWMTrafficManagerObserver

- (void)onTrafficStateUpdated { [self refreshAppearance]; }
@end
