#import "MWMTrafficButtonViewController.h"

#import <CoreApi/MWMMapOverlayManager.h>

#import "MWMAlertViewController.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"
#import "SwiftBridge.h"
#import "base/assert.hpp"
namespace {
CGFloat const kTopOffset = 6;

NSArray<UIImage *> *imagesWithName(NSString *name) {
  NSUInteger const imagesCount = 3;
  NSMutableArray<UIImage *> *images = [NSMutableArray arrayWithCapacity:imagesCount];
  NSString *mode = [UIColor isNightMode] ? @"dark" : @"light";
  for (NSUInteger i = 1; i <= imagesCount; i += 1) {
    NSString *imageName = [NSString stringWithFormat:@"%@_%@_%@", name, mode, @(i).stringValue];
    [images addObject:static_cast<UIImage *_Nonnull>([UIImage imageNamed:imageName])];
  }
  return [images copy];
}
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMTrafficButtonViewController *trafficButton;

@end

@interface MWMTrafficButtonViewController () <MWMMapOverlayManagerObserver, ThemeListener>

@property(nonatomic) NSLayoutConstraint *topOffset;
@property(nonatomic) NSLayoutConstraint *leftOffset;
@property(nonatomic) CGRect availableArea;

@end

@implementation MWMTrafficButtonViewController

+ (MWMTrafficButtonViewController *)controller {
  return [MWMMapViewControlsManager manager].trafficButton;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    MapViewController *ovc = [MapViewController sharedController];
    [ovc addChildViewController:self];
    [ovc.controlsView addSubview:self.view];
    [self configLayout];
    [self applyTheme];
    [StyleManager.shared addListener:self];
    [MWMMapOverlayManager addObserver:self];
  }
  return self;
}

- (void)dealloc {
  [StyleManager.shared removeListener:self];
}

- (void)configLayout {
  UIView *sv = self.view;
  UIView *ov = sv.superview;

  self.topOffset = [sv.topAnchor constraintEqualToAnchor:ov.topAnchor constant:kTopOffset];
  self.topOffset.active = YES;
  self.leftOffset = [sv.leadingAnchor constraintEqualToAnchor:ov.leadingAnchor constant:kViewControlsOffsetToBounds];
  self.leftOffset.active = YES;
}

- (void)setHidden:(BOOL)hidden {
  _hidden = hidden;
  [self refreshLayout];
}

- (void)refreshLayout {
  dispatch_async(dispatch_get_main_queue(), ^{
    auto const availableArea = self.availableArea;
    auto const leftOffset = self.hidden ? -self.view.width : availableArea.origin.x + kViewControlsOffsetToBounds;
    [self.view.superview animateConstraintsWithAnimations:^{
      self.topOffset.constant = availableArea.origin.y + kTopOffset;
      self.leftOffset.constant = leftOffset;
    }];
  });
}

- (void)applyTheme {
  MWMButton *btn = static_cast<MWMButton *>(self.view);
  UIImageView *iv = btn.imageView;

  // Traffic state machine: https://confluence.mail.ru/pages/viewpage.action?pageId=103680959
  [iv stopAnimating];
  if ([MWMMapOverlayManager trafficEnabled]) {
    switch ([MWMMapOverlayManager trafficState]) {
      case MWMMapOverlayTrafficStateDisabled:
        CHECK(false, ("Incorrect traffic manager state."));
        break;
      case MWMMapOverlayTrafficStateEnabled:
        btn.imageName = @"btn_traffic_on";
        break;
      case MWMMapOverlayTrafficStateWaitingData:
        iv.animationImages = imagesWithName(@"btn_traffic_update");
        iv.animationDuration = 0.8;
        [iv startAnimating];
        break;
      case MWMMapOverlayTrafficStateOutdated:
        btn.imageName = @"btn_traffic_outdated";
        break;
      case MWMMapOverlayTrafficStateNoData:
        btn.imageName = @"btn_traffic_on";
        [[MWMToast toastWithText:L(@"traffic_data_unavailable")] show];
        break;
      case MWMMapOverlayTrafficStateNetworkError:
        [MWMMapOverlayManager setTrafficEnabled:NO];
        [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
        break;
      case MWMMapOverlayTrafficStateExpiredData:
        btn.imageName = @"btn_traffic_outdated";
        [[MWMToast toastWithText:L(@"traffic_update_maps_text")] show];
        break;
      case MWMMapOverlayTrafficStateExpiredApp:
        btn.imageName = @"btn_traffic_outdated";
        [[MWMToast toastWithText:L(@"traffic_update_app_message")] show];
        break;
    }
  } else if ([MWMMapOverlayManager transitEnabled]) {
    btn.imageName = @"btn_subway_on";
    if ([MWMMapOverlayManager transitState] == MWMMapOverlayTransitStateNoData)
      [[MWMToast toastWithText:L(@"subway_data_unavailable")] show];
  } else if ([MWMMapOverlayManager isoLinesEnabled]) {
    btn.imageName = @"btn_isoMap_on";
    if ([MWMMapOverlayManager isolinesState] == MWMMapOverlayIsolinesStateNoData)
      [[MWMToast toastWithText:L(@"isolines_location_error_dialog")] show];
    else if ([MWMMapOverlayManager isolinesState] == MWMMapOverlayIsolinesStateExpiredData)
      [MWMAlertViewController.activeAlertController presentInfoAlert:L(@"isolines_activation_error_dialog") text:@""];
  } else if ([MWMMapOverlayManager guidesEnabled]) {
    btn.imageName = @"btn_layers_off";
    if ([MWMMapOverlayManager guidesState] == MWMMapOverlayGuidesStateNoData)
      [[MWMToast toastWithText:L(@"no_routes_in_the_area_toast")] show];
    else if ([MWMMapOverlayManager guidesState] == MWMMapOverlayGuidesStateNetworkError) {
      [MWMMapOverlayManager setGuidesEnabled:NO];
      [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
    }
  } else {
    btn.imageName = @"btn_layers";
  }
}

- (IBAction)buttonTouchUpInside {
  if ([MWMMapOverlayManager trafficEnabled]) {
    [MWMMapOverlayManager setTrafficEnabled:NO];
  } else if ([MWMMapOverlayManager transitEnabled]) {
    [MWMMapOverlayManager setTransitEnabled:NO];
  } else if ([MWMMapOverlayManager isoLinesEnabled]) {
    [MWMMapOverlayManager setIsoLinesEnabled:NO];
  } else if ([MWMMapOverlayManager guidesEnabled]) {
    [MWMMapOverlayManager setGuidesEnabled:NO];
  } else {
    MWMMapViewControlsManager.manager.menuState = MWMBottomMenuStateActive;
  }
}

+ (void)updateAvailableArea:(CGRect)frame {
  auto controller = [self controller];
  if (CGRectEqualToRect(controller.availableArea, frame))
    return;
  controller.availableArea = frame;
  [controller refreshLayout];
}

#pragma mark - MWMMapOverlayManagerObserver

- (void)onTrafficStateUpdated {
  [self applyTheme];
}
- (void)onTransitStateUpdated {
  [self applyTheme];
}
- (void)onIsoLinesStateUpdated {
  [self applyTheme];
}
- (void)onGuidesStateUpdated {
  [self applyTheme];
}

@end
