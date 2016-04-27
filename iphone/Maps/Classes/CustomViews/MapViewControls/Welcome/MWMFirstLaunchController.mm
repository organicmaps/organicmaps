#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMFirstLaunchController.h"
#import "MWMPageController.h"

#include "Framework.h"

@interface MWMFirstLaunchController () <LocationObserver>

@property (weak, nonatomic) IBOutlet UIView * containerView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * alertTitle;
@property (weak, nonatomic) IBOutlet UILabel * alertText;
@property (weak, nonatomic) IBOutlet UIButton * nextPageButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * containerHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageMinHeight;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * imageHeight;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleTopOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleImageOffset;

@property (nonatomic) BOOL locationError;

@end

namespace
{
void requestNotifications()
{
  UIApplication * app = [UIApplication sharedApplication];
  UIUserNotificationType userNotificationTypes =
      (UIUserNotificationTypeAlert | UIUserNotificationTypeBadge | UIUserNotificationTypeSound);
  if ([app respondsToSelector:@selector(registerUserNotificationSettings:)])
  {
    UIUserNotificationSettings * settings =
        [UIUserNotificationSettings settingsForTypes:userNotificationTypes categories:nil];
    [app registerUserNotificationSettings:settings];
    [app registerForRemoteNotifications];
  }
  else
  {
    [app registerForRemoteNotificationTypes:userNotificationTypes];
  }
}

void zoomToCurrentPosition()
{
  auto & f = GetFramework();
  f.SwitchMyPositionNextMode();
  LocationManager * locationManager = MapsAppDelegate.theApp.locationManager;
  if (![locationManager lastLocationIsValid])
    return;
  m2::PointD const centerPt = locationManager.lastLocation.mercator;
  int const zoom = 13;
  f.SetViewportCenter(centerPt, zoom);
}

NSInteger constexpr kRequestLocationPage = 2;
NSInteger constexpr kRequestNotificationsPage = 3;

NSArray<TMWMWelcomeConfigBlock> * pagesConfigBlocks = @[
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_offline_maps"];
    controller.alertTitle.text = L(@"onboarding_offline_maps_title");
    controller.alertText.text = L(@"onboarding_offline_maps_message");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_geoposition"];
    controller.alertTitle.text = L(@"onboarding_location_title");
    controller.alertText.text = L(@"onboarding_location_message");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_notification"];
    controller.alertTitle.text = L(@"onboarding_notifications_title");
    controller.alertText.text = L(@"onboarding_notifications_message");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_done"];
    controller.alertTitle.text = L(@"first_launch_congrats_title");
    controller.alertText.text = L(@"first_launch_congrats_text");
    [controller.nextPageButton setTitle:L(@"done") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller
                                  action:@selector(close)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy]
];
} // namespace

@implementation MWMFirstLaunchController

+ (NSString *)udWelcomeWasShownKey
{
  return @"FirstLaunchWelcomeWasShown";
}

+ (NSArray<TMWMWelcomeConfigBlock> *)pagesConfig
{
  return pagesConfigBlocks;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (self.pageIndex == kRequestLocationPage)
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(appWillEnterForeground:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (self.pageIndex == kRequestLocationPage)
    [self requestLocation];
  else if (self.pageIndex == kRequestNotificationsPage)
    requestNotifications();
}

- (void)viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];
  if (self.locationError)
    [MapsAppDelegate.theApp.locationManager reset];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)requestLocation
{
  MapsAppDelegate * app = MapsAppDelegate.theApp;
  LocationManager * lm = app.locationManager;
  [lm onForeground];
  [lm start:self];
}

- (void)close
{
  [self.pageController close];
  zoomToCurrentPosition();
}

- (void)appWillEnterForeground:(NSNotification *)notification
{
  [self requestLocation];
}

#pragma mark - LocationManager Callbacks

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  if (errorCode == location::EDenied)
    self.locationError = YES;
}

#pragma mark - Properties

- (void)setSize:(CGSize)size
{
  super.size = size;
  CGSize const newSize = super.size;
  CGFloat const width = newSize.width;
  CGFloat const height = newSize.height;
  BOOL const hideImage = (self.imageHeight.multiplier * height <= self.imageMinHeight.constant);
  self.titleImageOffset.priority = hideImage ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
  self.image.hidden = hideImage;
  self.containerWidth.constant = width;
  self.containerHeight.constant = height;
}

@end
