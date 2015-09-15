#import "BookmarksRootVC.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMActivityViewController.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMSideMenuButton.h"
#import "MWMSideMenuButtonDelegate.h"
#import "MWMSideMenuDelegate.h"
#import "MWMSideMenuDownloadBadge.h"
#import "MWMSideMenuManager.h"
#import "MWMSideMenuManagerDelegate.h"
#import "MWMSideMenuView.h"
#import "SettingsAndMoreVC.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"
#include "map/information_display.hpp"

static NSString * const kMWMSideMenuViewsNibName = @"MWMSideMenuViews";

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSideMenuManager() <MWMSideMenuInformationDisplayProtocol, MWMSideMenuButtonTapProtocol,
                                 MWMSideMenuButtonLayoutProtocol>

@property (weak, nonatomic) id<MWMSideMenuManagerProtocol> delegate;
@property (weak, nonatomic) MapViewController * controller;

@property (nonatomic) IBOutlet MWMSideMenuButton * menuButton;
@property (nonatomic) IBOutlet MWMSideMenuDownloadBadge * downloadBadge;
@property (nonatomic) IBOutlet MWMSideMenuView * sideMenu;

@property (weak, nonatomic) IBOutlet UIButton * shareLocationButton;

@end

@implementation MWMSideMenuManager

- (instancetype)initWithParentController:(MapViewController *)controller
                                delegate:(id<MWMSideMenuManagerProtocol>)delegate
{
  self = [super init];
  if (self)
  {
    self.controller = controller;
    self.delegate = delegate;
    [[NSBundle mainBundle] loadNibNamed:kMWMSideMenuViewsNibName owner:self options:nil];
    [self.controller.view addSubview:self.menuButton];
    [self.menuButton setup];
    self.menuButton.delegate = self;
    self.sideMenu.delegate = self;
    [self addCloseMenuWithTap];
    self.state = MWMSideMenuStateInactive;
  }
  return self;
}

- (void)addCloseMenuWithTap
{
  UITapGestureRecognizer * const tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMenu)];
  [self.sideMenu.dimBackground addGestureRecognizer:tap];
}

#pragma mark - Actions

- (IBAction)menuActionOpenBookmarks
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  BookmarksRootVC * const vc = [[BookmarksRootVC alloc] init];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionDownloadMaps
{
  [self.delegate actionDownloadMaps];
}

- (IBAction)menuActionOpenSettings
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  SettingsAndMoreVC * const vc = [[SettingsAndMoreVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionShareLocation
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"share@"];
  CLLocation const * const location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location)
  {
    [[[UIAlertView alloc] initWithTitle:L(@"unknown_current_position") message:nil delegate:nil
                      cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
    return;
  }
  CLLocationCoordinate2D const coord = location.coordinate;
  MWMActivityViewController * shareVC = [MWMActivityViewController shareControllerForLocationTitle:nil location:coord
                                                                                        myPosition:YES];
  [shareVC presentInParentViewController:self.controller anchorView:self.shareLocationButton];
}

- (IBAction)menuActionOpenSearch
{
  self.state = MWMSideMenuStateInactive;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  self.controller.controlsManager.searchHidden = NO;
}

- (void)toggleMenu
{
  if (self.state == MWMSideMenuStateActive)
    self.state = MWMSideMenuStateInactive;
  else if (self.state == MWMSideMenuStateInactive)
    self.state = MWMSideMenuStateActive;
}

#pragma mark - MWMSideMenuButtonTapProtocol

- (void)handleSingleTap
{
  [self toggleMenu];
}

- (void)handleDoubleTap
{
  [self menuActionOpenSearch];
}

#pragma mark - MWMSideMenuButtonLayoutProtocol

- (void)menuButtonDidUpdateLayout
{
  [self.delegate sideMenuDidUpdateLayout];
}

#pragma mark - MWMSideMenuInformationDisplayProtocol

- (void)setRulerPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::Ruler, pivot);
  });
}

- (void)setCopyrightLabelPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::CopyrightLabel, pivot);
  });
}

- (void)showMenu
{
  self.menuButton.alpha = 1.0;
  self.sideMenu.alpha = 0.0;
  [self.controller.view addSubview:self.sideMenu];
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 0.0;
    self.sideMenu.alpha = 1.0;
  }
  completion:^(BOOL finished)
  {
    if (self.state == MWMSideMenuStateActive)
      [self.menuButton setHidden:YES animated:NO];
  }];
}

- (void)hideMenu
{
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 1.0;
    self.sideMenu.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    [self.sideMenu removeFromSuperview];
  }];
}

- (void)addDownloadBadgeToView:(UIView<MWMSideMenuDownloadBadgeOwner> *)view
{
  int const count = GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount();
  if (count > 0)
  {
    self.downloadBadge.outOfDateCount = count;
    view.downloadBadge = self.downloadBadge;
    [self.downloadBadge showAnimatedAfterDelay:framesDuration(10)];
  }
}

#pragma mark - Properties

- (void)setState:(MWMSideMenuState)state
{
  if (_state == state)
    return;
  [self.downloadBadge hide];
  switch (state)
  {
    case MWMSideMenuStateActive:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"menu"];
      [self addDownloadBadgeToView:self.sideMenu];
      [self showMenu];
      [self.sideMenu setup];
      break;
    case MWMSideMenuStateInactive:
      [self addDownloadBadgeToView:self.menuButton];
      if (_state == MWMSideMenuStateActive)
      {
        [self.menuButton setHidden:NO animated:NO];
        [self hideMenu];
      }
      else
      {
        [self.menuButton setHidden:NO animated:YES];
      }
      break;
    case MWMSideMenuStateHidden:
      [self.menuButton setHidden:YES animated:YES];
      [self hideMenu];
      break;
  }
  _state = state;
  [self.controller updateStatusBarStyle];
}

- (CGRect)menuButtonFrameWithSpacing
{
  return self.menuButton.frameWithSpacing;
}

@end
