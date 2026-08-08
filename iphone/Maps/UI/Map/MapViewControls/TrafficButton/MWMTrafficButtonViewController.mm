#import "MWMTrafficButtonViewController.h"

#import <CoreApi/MWMMapOverlayManager.h>
#import <CoreApi/MWMStorage.h>

#import "MWMAlertViewController.h"
#import "MWMButton.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MapViewController.h"
#import "SwiftBridge.h"
#import "base/assert.hpp"

namespace
{
CGFloat const kTopOffset = 6;

NSArray<UIImage *> * imagesWithName(NSString * name)
{
  NSUInteger const imagesCount = 3;
  NSMutableArray<UIImage *> * images = [NSMutableArray arrayWithCapacity:imagesCount];
  for (NSUInteger i = 1; i <= imagesCount; i += 1)
  {
    NSString * imageName = [NSString stringWithFormat:@"%@_%@", name, @(i).stringValue];
    [images addObject:static_cast<UIImage * _Nonnull>([UIImage imageNamed:imageName])];
  }
  return [images copy];
}
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMTrafficButtonViewController * trafficButton;

@end

@interface MWMTrafficButtonViewController () <MWMMapOverlayManagerObserver, ThemeListener>

@property(nonatomic) NSLayoutConstraint * topOffset;
@property(nonatomic) NSLayoutConstraint * leftOffset;
@property(nonatomic) CGRect availableArea;
@property(nonatomic) MWMMapOverlayIsolinesState lastHandledIsolinesState;

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
    MapViewController * ovc = [MapViewController sharedController];
    [ovc addChildViewController:self];
    [ovc.controlsView addSubview:self.view];
    [self configLayout];
    [self applyTheme];
    [StyleManager.shared addListener:self];
    [MWMMapOverlayManager addObserver:self];
  }
  return self;
}

- (void)dealloc
{
  [StyleManager.shared removeListener:self];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  [Toast hideAll];
}

- (void)configLayout
{
  UIView * sv = self.view;
  UIView * ov = sv.superview;

  self.topOffset = [sv.topAnchor constraintEqualToAnchor:ov.topAnchor constant:kTopOffset];
  self.topOffset.active = YES;
  self.leftOffset = [sv.leadingAnchor constraintEqualToAnchor:ov.leadingAnchor constant:kViewControlsOffsetToBounds];
  self.leftOffset.active = YES;
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
    auto const fitInAvailableArea = CGRectGetMaxY(self.view.frame) < CGRectGetMaxY(availableArea) + kTopOffset;
    auto const shouldHide = self.hidden || !fitInAvailableArea;
    auto const leftOffset = shouldHide ? -self.view.width : availableArea.origin.x + kViewControlsOffsetToBounds;
    [self.view.superview animateConstraintsWithAnimations:^{
      self.topOffset.constant = availableArea.origin.y + kTopOffset;
      self.leftOffset.constant = leftOffset;
      self.view.alpha = shouldHide ? 0 : 1;
    }];
  });
}

- (void)handleTrafficState:(MWMMapOverlayTrafficState)state
{
  MWMButton * btn = (MWMButton *)self.view;
  UIImageView * iv = btn.imageView;
  switch (state)
  {
  case MWMMapOverlayTrafficStateDisabled: CHECK(false, ("Incorrect traffic manager state.")); break;
  case MWMMapOverlayTrafficStateEnabled: btn.imageName = @"btn_traffic_on"; break;
  case MWMMapOverlayTrafficStateWaitingData:
    iv.animationImages = imagesWithName(@"btn_traffic_update");
    iv.animationDuration = 0.8;
    [iv startAnimating];
    break;
  case MWMMapOverlayTrafficStateOutdated: btn.imageName = @"btn_traffic_outdated"; break;
  case MWMMapOverlayTrafficStateNoData:
    btn.imageName = @"btn_traffic_on";
    [Toast showWithText:L(@"traffic_data_unavailable")];
    break;
  case MWMMapOverlayTrafficStateNetworkError:
    [MWMMapOverlayManager setTrafficEnabled:NO];
    [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
    break;
  case MWMMapOverlayTrafficStateExpiredData:
    btn.imageName = @"btn_traffic_outdated";
    [Toast showWithText:L(@"traffic_update_maps_text")];
    break;
  case MWMMapOverlayTrafficStateExpiredApp:
    btn.imageName = @"btn_traffic_outdated";
    [Toast showWithText:L(@"traffic_update_app_message")];
    break;
  }
}

- (void)handleIsolinesState:(MWMMapOverlayIsolinesState)state
{
  // applyTheme re-derives the state on every theme/overlay poke: the hints must fire
  // on the actual transitions only, or they would stack over an unchanged NoData.
  if (state == self.lastHandledIsolinesState)
    return;
  self.lastHandledIsolinesState = state;
  switch (state)
  {
  case MWMMapOverlayIsolinesStateDisabled: break;
  case MWMMapOverlayIsolinesStateEnabled:
    if (![MWMMapOverlayManager isolinesVisible])
      [Toast showWithText:L(@"isolines_toast_zooms_1_10")];
    break;
  case MWMMapOverlayIsolinesStateNoData:
    // Keep the layer enabled: the hint prompts to download the terrain, and the state
    // recovers by itself once the viewport gets the coverage. A passive toast, like on
    // Android: nothing to decide here, a modal alert would only get in the way.
    if (![[MWMStorage sharedStorage] isTerrainAvailable] || [[MWMStorage sharedStorage] isTerrainWithMaps])
    {
      [Toast showWithText:L(@"isolines_location_error_dialog")];
    }
    else
    {
      // The terrain does not arrive with the maps: offer to turn the setting on and to
      // fetch the terrain of the downloaded regions in the current viewport.
      [MWMAlertViewController.activeAlertController presentDefaultAlertWithTitle:L(@"terrain_disabled_dialog")
                                                                         message:nil
                                                                rightButtonTitle:L(@"enable")
                                                                 leftButtonTitle:L(@"cancel")
                                                               rightButtonAction:^{
                                                                 MWMStorage * storage = [MWMStorage sharedStorage];
                                                                 [storage setTerrainWithMaps:YES];
                                                                 [storage downloadTerrainForViewport];  // MWMStorage+UI
                                                               }];
    }
    break;
  }
}

- (void)applyTheme
{
  MWMButton * btn = static_cast<MWMButton *>(self.view);
  UIImageView * iv = btn.imageView;

  // Traffic state machine: https://confluence.mail.ru/pages/viewpage.action?pageId=103680959
  [iv stopAnimating];
  // The transition gate must see the layer going off, or the next NoData after a
  // re-enable would read as a repeat and the alerts would stay silent.
  if (![MWMMapOverlayManager isoLinesEnabled])
    self.lastHandledIsolinesState = MWMMapOverlayIsolinesStateDisabled;
  if ([MWMMapOverlayManager trafficEnabled])
  {
    [self handleTrafficState:[MWMMapOverlayManager trafficState]];
  }
  else if ([MWMMapOverlayManager transitEnabled])
  {
    btn.imageName = @"btn_subway_on";
    if ([MWMMapOverlayManager transitState] == MWMMapOverlayTransitStateNoData)
      [Toast showWithText:L(@"subway_data_unavailable")];
  }
  else if ([MWMMapOverlayManager isoLinesEnabled])
  {
    btn.imageName = @"btn_isoMap_on";
    [self handleIsolinesState:[MWMMapOverlayManager isolinesState]];
  }
  else if ([MWMMapOverlayManager outdoorEnabled] || [MWMMapOverlayManager hikingEnabled] ||
           [MWMMapOverlayManager cyclingEnabled])
  {
    btn.imageName = @"btn_isoMap_on";
  }
  else
  {
    btn.imageName = @"btn_layers";
  }
}

- (IBAction)buttonTouchUpInside
{
  BOOL needsToDisableMapLayer = [MWMMapOverlayManager trafficEnabled] || [MWMMapOverlayManager transitEnabled] ||
                                [MWMMapOverlayManager isoLinesEnabled] || [MWMMapOverlayManager outdoorEnabled] ||
                                [MWMMapOverlayManager hikingEnabled] || [MWMMapOverlayManager cyclingEnabled];

  if (needsToDisableMapLayer)
  {
    [MWMMapOverlayManager setTrafficEnabled:NO];
    [MWMMapOverlayManager setTransitEnabled:NO];
    [MWMMapOverlayManager setIsoLinesEnabled:NO];
    [MWMMapOverlayManager setOutdoorEnabled:NO];
    [MWMMapOverlayManager setHikingEnabled:NO];
    [MWMMapOverlayManager setCyclingEnabled:NO];
  }
  else
  {
    MWMMapViewControlsManager.manager.menuState = MWMBottomMenuStateLayers;
  }
}

+ (void)updateAvailableArea:(CGRect)frame
{
  auto controller = [self controller];
  if (CGRectEqualToRect(controller.availableArea, frame))
    return;
  controller.availableArea = frame;
  [controller refreshLayout];
}

#pragma mark - MWMMapOverlayManagerObserver

- (void)onMapOverlayUpdated
{
  [self applyTheme];
}

@end
