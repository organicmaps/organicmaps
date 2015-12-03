#import "Common.h"
#import "LocalNotificationManager.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "TimeUtils.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

#include "platform/platform.hpp"
#include "storage/storage_defines.hpp"

static NSString * const kDownloadMapActionName = @"DownloadMapAction";

static NSString * const kFlagsKey = @"DownloadMapNotificationFlags";
static double constexpr kRepeatedNotificationIntervalInSeconds = 3 * 30 * 24 * 60 * 60; // three months

using namespace storage;

@interface LocalNotificationManager () <CLLocationManagerDelegate, UIAlertViewDelegate>

@property (nonatomic) CLLocationManager * locationManager;
@property (nonatomic) TIndex countryIndex;
@property (copy, nonatomic) CompletionHandler downloadMapCompletionHandler;
@property (weak, nonatomic) NSTimer * timer;

@end

@implementation LocalNotificationManager

+ (instancetype)sharedManager
{
  static LocalNotificationManager * manager = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^
  {
    manager = [[self alloc] init];
  });
  return manager;
}

- (void)dealloc
{
  _locationManager.delegate = nil;
}

- (void)processNotification:(UILocalNotification *)notification onLaunch:(BOOL)onLaunch
{
  NSDictionary * userInfo = [notification userInfo];
  if ([userInfo[@"Action"] isEqualToString:kDownloadMapActionName])
  {
    [[Statistics instance] logEvent:@"'Download Map' Notification Clicked"];
    [[MapsAppDelegate theApp].mapViewController.navigationController popToRootViewControllerAnimated:NO];

    TIndex const index = TIndex([userInfo[@"Group"] intValue], [userInfo[@"Country"] intValue], [userInfo[@"Region"] intValue]);
    [self downloadCountryWithIndex:index];
  }
}

#pragma mark - Location Notifications

- (void)showDownloadMapNotificationIfNeeded:(CompletionHandler)completionHandler
{
  NSTimeInterval const completionTimeIndent = 2.0;
  NSTimeInterval const backgroundTimeRemaining = UIApplication.sharedApplication.backgroundTimeRemaining - completionTimeIndent;
  if ([CLLocationManager locationServicesEnabled] && backgroundTimeRemaining > 0.0)
  {
    self.downloadMapCompletionHandler = completionHandler;
    self.timer = [NSTimer scheduledTimerWithTimeInterval:backgroundTimeRemaining target:self selector:@selector(timerSelector:) userInfo:nil repeats:NO];
    [self.locationManager startUpdatingLocation];
  }
  else
  {
    [self.locationManager stopUpdatingLocation];
    completionHandler(UIBackgroundFetchResultFailed);
  }
}

- (void)markNotificationShowingForIndex:(TIndex)index
{
  NSMutableDictionary * flags = [[[NSUserDefaults standardUserDefaults] objectForKey:kFlagsKey] mutableCopy];
  if (!flags)
    flags = [[NSMutableDictionary alloc] init];
  
  flags[[self flagStringForIndex:index]] = [NSDate date];
  
  NSUserDefaults * userDefaults = [NSUserDefaults standardUserDefaults];
  [userDefaults setObject:flags forKey:kFlagsKey];
  [userDefaults synchronize];
}

- (BOOL)shouldShowNotificationForIndex:(TIndex)index
{
  NSDictionary * flags = [[NSUserDefaults standardUserDefaults] objectForKey:kFlagsKey];
  NSDate * lastShowDate = flags[[self flagStringForIndex:index]];
  return !lastShowDate || [[NSDate date] timeIntervalSinceDate:lastShowDate] > kRepeatedNotificationIntervalInSeconds;
}

- (void)timerSelector:(id)sender
{
  // Location still was not received but it's time to finish up so system will not kill us.
  [self.locationManager stopUpdatingLocation];
  [self performCompletionHandler:UIBackgroundFetchResultFailed];
}

- (void)downloadCountryWithIndex:(TIndex)index
{
  /// @todo Fix this logic after Framework -> CountryTree -> ActiveMapLayout refactoring.
  /// Call download via Framework.
  Framework & f = GetFramework();
  f.GetActiveMaps()->DownloadMap(index, MapOptions::Map);
  double const defaultZoom = 10;
  f.ShowRect(f.GetCountryBounds(index), defaultZoom);
}

- (NSString *)flagStringForIndex:(TIndex)index
{
  return [NSString stringWithFormat:@"%i_%i_%i", index.m_group, index.m_country, index.m_region];
}

- (TIndex)indexWithFlagString:(NSString *)flag
{
  NSArray * components = [flag componentsSeparatedByString:@"_"];
  if ([components count] == 3)
    return TIndex([components[0] intValue], [components[1] intValue], [components[2] intValue]);
  
  return TIndex();
}

- (void)performCompletionHandler:(UIBackgroundFetchResult)result
{
  if (!self.downloadMapCompletionHandler)
    return;
  self.downloadMapCompletionHandler(result);
  self.downloadMapCompletionHandler = nil;
}

#pragma mark - Location Manager

- (CLLocationManager *)locationManager
{
  if (!_locationManager)
  {
    _locationManager = [[CLLocationManager alloc] init];
    _locationManager.delegate = self;
    _locationManager.distanceFilter = kCLLocationAccuracyThreeKilometers;
  }
  return _locationManager;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations
{
  [self.timer invalidate];
  [self.locationManager stopUpdatingLocation];
  NSString * flurryEventName = @"'Download Map' Notification Didn't Schedule";
  UIBackgroundFetchResult result = UIBackgroundFetchResultNoData;

  BOOL const inBackground = [UIApplication sharedApplication].applicationState == UIApplicationStateBackground;
  BOOL const onWiFi = (Platform::ConnectionStatus() == Platform::EConnectionType::CONNECTION_WIFI);
  if (inBackground && onWiFi)
  {
    Framework & f = GetFramework();
    CLLocation * lastLocation = [locations lastObject];
    TIndex const index = f.GetCountryIndex(lastLocation.mercator);
    
    if (index.IsValid() && [self shouldShowNotificationForIndex:index])
    {
      TStatus const status = f.GetCountryStatus(index);
      if (status == TStatus::ENotDownloaded)
      {
        [self markNotificationShowingForIndex:index];
        
        UILocalNotification * notification = [[UILocalNotification alloc] init];
        notification.alertAction = L(@"download");
        notification.alertBody = L(@"download_map_notification");
        notification.soundName = UILocalNotificationDefaultSoundName;
        notification.userInfo = @{@"Action" : kDownloadMapActionName, @"Group" : @(index.m_group), @"Country" : @(index.m_country), @"Region" : @(index.m_region)};
        
        UIApplication * application = [UIApplication sharedApplication];
        [application presentLocalNotificationNow:notification];

        [Alohalytics logEvent:@"suggestedToDownloadMissingMapForCurrentLocation" atLocation:lastLocation];
        flurryEventName = @"'Download Map' Notification Scheduled";
        result = UIBackgroundFetchResultNewData;
      }
    }
  }
  [[Statistics instance] logEvent:flurryEventName withParameters:@{@"WiFi" : @(onWiFi)}];
  [self performCompletionHandler:result];
}

@end
