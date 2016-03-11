#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMFirstLaunchController.h"
#import "MWMPageController.h"

#include "Framework.h"

@interface MWMFirstLaunchController ()

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

@end

namespace
{
void requestLocation()
{
  GetFramework().SwitchMyPositionNextMode();
}

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
  dispatch_async(dispatch_get_main_queue(), ^
  {
    LocationManager * locationManager = MapsAppDelegate.theApp.m_locationManager;
    if (![locationManager lastLocationIsValid])
      return;
    m2::PointD const centerPt = locationManager.lastLocation.mercator;
    int const zoom = 13;
    bool const isAnim = true;
    GetFramework().GetDrapeEngine()->SetModelViewCenter(centerPt, zoom, isAnim);
  });
}

NSArray<TMWMWelcomeConfigBlock> * pagesConfigBlocks = @[
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_offline_maps"];
    controller.alertTitle.text = L(@"first_launch_welcome_title");
    controller.alertText.text = L(@"first_launch_welcome_text");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_geoposition"];
    controller.alertTitle.text = L(@"first_launch_need_location_title");
    controller.alertText.text = L(@"first_launch_need_location_text");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"img_onboarding_notification"];
    controller.alertTitle.text = L(@"first_launch_need_push_title");
    controller.alertText.text = L(@"first_launch_need_push_text");
    [controller.nextPageButton setTitle:L(@"whats_new_next_button") forState:UIControlStateNormal];
    [controller.nextPageButton addTarget:controller.pageController
                                  action:@selector(nextPage)
                        forControlEvents:UIControlEventTouchUpInside];
  } copy],
  [^(MWMFirstLaunchController * controller)
  {
    controller.image.image = [UIImage imageNamed:@"ic_placeholder"];
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

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (self.pageIndex == 2)
    requestLocation();
  else if (self.pageIndex == 3)
    requestNotifications();
}

- (void)close
{
  [self.pageController close];
  zoomToCurrentPosition();
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
