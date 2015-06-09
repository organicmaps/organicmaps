#import "AppInfo.h"
#import "Common.h"
#import "Framework.h"
#import "LocalNotificationInfoProvider.h"
#import "LocalNotificationManager.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "TimeUtils.h"
#import "UIKitCategories.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "storage/storage_defines.hpp"

#define DOWNLOAD_MAP_ACTION_NAME @"DownloadMapAction"

#define FLAGS_KEY @"DownloadMapNotificationFlags"
#define SHOW_INTERVAL (3 * 30 * 24 * 60 * 60) // three months

NSString * const LocalNotificationManagerSpecialNotificationInfoKey = @"LocalNotificationManagerSpecialNotificationInfoKey";
NSString * const LocalNotificationManagerNumberOfViewsPrefix = @"LocalNotificationManagerNumberOfViewsPrefix";

using namespace storage;

typedef void (^CompletionHandler)(UIBackgroundFetchResult);

@interface LocalNotificationManager () <CLLocationManagerDelegate, UIAlertViewDelegate>

@property (nonatomic) CLLocationManager * locationManager;
@property (nonatomic) TIndex countryIndex;
@property (nonatomic, copy) CompletionHandler downloadMapCompletionHandler;
@property (nonatomic) NSTimer * timer;

@end

@implementation LocalNotificationManager

+ (instancetype)sharedManager
{
  static LocalNotificationManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[self alloc] init];
  });
  return manager;
}

- (void)updateLocalNotifications
{
  [self scheduleSpecialLocalNotifications];
}

- (void)processNotification:(UILocalNotification *)notification onLaunch:(BOOL)onLaunch
{
  NSDictionary * userInfo = [notification userInfo];
  if ([userInfo[@"Action"] isEqualToString:DOWNLOAD_MAP_ACTION_NAME])
  {
    [[Statistics instance] logEvent:@"'Download Map' Notification Clicked"];
    [[MapsAppDelegate theApp].m_mapViewController.navigationController popToRootViewControllerAnimated:NO];

    TIndex const index = TIndex([userInfo[@"Group"] intValue], [userInfo[@"Country"] intValue], [userInfo[@"Region"] intValue]);
    [self downloadCountryWithIndex:index];
  }
  else if (userInfo[LocalNotificationManagerSpecialNotificationInfoKey])
  {
    NSDictionary * notificationInfo = userInfo[LocalNotificationManagerSpecialNotificationInfoKey];
    if (onLaunch)
      [self runNotificationAction:notificationInfo];
    else
    {
      NSString * dismissiveAction = L(@"later");
      NSString * positiveAction = [self actionTitleWithAction:notificationInfo[@"NotificationAction"]];
      NSString * notificationTitle = L(notificationInfo[@"NotificationLocalizedAlertBodyKey"]);
      if (![notificationTitle length])
        notificationTitle = L(notificationInfo[@"NotificationLocalizedBodyKey"]);
      UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:notificationTitle message:nil delegate:nil cancelButtonTitle:dismissiveAction otherButtonTitles:positiveAction, nil];
      alertView.tapBlock = ^(UIAlertView *alertView, NSInteger buttonIndex) {
        NSString * notificationID = notificationInfo[@"NotificationID"];
        BOOL shared = (buttonIndex != alertView.cancelButtonIndex);
        [[Statistics instance] logEvent:[NSString stringWithFormat:@"'%@' Notification Show", notificationID] withParameters:@{@"Shared" : @(shared)}];
        if (shared)
          [self runNotificationAction:notificationInfo];
      };
      [alertView show];
    }
  }
}

- (NSString *)actionTitleWithAction:(NSString *)action
{
  if ([action isEqualToString:@"Share"])
    return L(@"share");
  else if ([action isEqualToString:@"AppStoreProVersion"])
    return L(@"download");
  else
    return nil;
}

- (void)runNotificationAction:(NSDictionary *)notificationInfo
{
  NSString * action = notificationInfo[@"NotificationAction"];
  
  if ([action isEqualToString:@"Share"])
  {
    NSURL * link = [NSURL URLWithString:notificationInfo[@"NotificationShareLink"]];
    UIImage * shareImage = [UIImage imageNamed:notificationInfo[@"NotifiicationShareImage"]];
    LocalNotificationInfoProvider * infoProvider = [[LocalNotificationInfoProvider alloc] initWithDictionary:notificationInfo];

    if (isIOSVersionLessThan(6))
    {
      UIPasteboard * pasteboard = [UIPasteboard generalPasteboard];
      pasteboard.URL = link;
      NSString * message = [NSString stringWithFormat:L(@"copied_to_clipboard"), [link absoluteString]];
      UIAlertView * alertView = [[UIAlertView alloc] initWithTitle:message message:nil delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil];
      [alertView show];
    }
    else
    {
      NSMutableArray * itemsToShare = [NSMutableArray arrayWithObject:infoProvider];
      if (shareImage)
        [itemsToShare addObject:shareImage];
      
      UIActivityViewController * activityVC = [[UIActivityViewController alloc] initWithActivityItems:itemsToShare applicationActivities:nil];
      NSMutableArray * excludedActivityTypes = [@[UIActivityTypePrint, UIActivityTypeAssignToContact, UIActivityTypeSaveToCameraRoll] mutableCopy];
      if (!isIOSVersionLessThan(7))
        [excludedActivityTypes addObject:UIActivityTypeAirDrop];
      activityVC.excludedActivityTypes = excludedActivityTypes;
      UIWindow * window = [[UIApplication sharedApplication].windows firstObject];
      NavigationController * vc = (NavigationController *)window.rootViewController;
      [vc presentViewController:activityVC animated:YES completion:nil];
    }
  }
}

#pragma mark - Special Notifications

- (BOOL)isSpecialLocalNotification:(UILocalNotification *)notification
{
  if (notification.userInfo && notification.userInfo[LocalNotificationManagerSpecialNotificationInfoKey])
    return YES;
  else
    return NO;
}

- (NSArray *)scheduledSpecialLocalNotifications
{
  NSArray * allNotifications = [[UIApplication sharedApplication] scheduledLocalNotifications];
  NSMutableArray * specialNotifications = [[NSMutableArray alloc] init];
  for (UILocalNotification * notification in allNotifications)
  {
    if ([self isSpecialLocalNotification:notification])
      [specialNotifications addObject:notification];
  }
  
  return specialNotifications;
}

- (BOOL)isSpecialNotificationScheduled:(NSString *)specialNotificationID
{
  NSArray * notifications = [self scheduledSpecialLocalNotifications];
  
  for (UILocalNotification * scheduledNotification in notifications)
  {
    NSDictionary * notificationInfo = scheduledNotification.userInfo[LocalNotificationManagerSpecialNotificationInfoKey];
    NSString * scheduledSpecialNotificationID = notificationInfo[@"NotificationID"];
    if ([scheduledSpecialNotificationID isEqualToString:specialNotificationID])
      return YES;
  }
  
  return NO;
}

- (void)increaseViewsNumberOfNotification:(NSString *)specialNotificationID
{
  NSUserDefaults * userDefaults = [NSUserDefaults standardUserDefaults];
  NSString * key = [NSString stringWithFormat:@"%@%@", LocalNotificationManagerNumberOfViewsPrefix, specialNotificationID];
  NSNumber * viewsNumber = [userDefaults objectForKey:key];
  viewsNumber = viewsNumber ? @([viewsNumber integerValue] + 1) : @(1);
  [userDefaults setObject:viewsNumber forKey:key];
  [userDefaults synchronize];
}

- (NSNumber *)viewNumberOfNotification:(NSString *)specialNotificationID
{
  NSUserDefaults * userDefaults = [NSUserDefaults standardUserDefaults];
  NSString * key = [NSString stringWithFormat:@"%@%@", LocalNotificationManagerNumberOfViewsPrefix, specialNotificationID];
  NSNumber * viewsNumber = [userDefaults objectForKey:key];
  if (!viewsNumber)
    viewsNumber = @(0);
  return viewsNumber;
}

- (void)scheduleSpecialLocalNotifications
{
  NSArray * localNotificationsInfo = [self localNotificationsInfo];
  NSMutableArray * actualSpecialLocalNotifications = [NSMutableArray array];
  for (NSDictionary * notificationInfo in localNotificationsInfo)
  {
    NSString * notificationID = notificationInfo[@"NotificationID"];
    if ([self isSpecialNotificationScheduled:notificationID])
      continue;
    
    NSNumber * viewsLimit = notificationInfo[@"NotificationViewsLimit"];
    NSNumber * viewsNumber = [self viewNumberOfNotification:notificationID];
    if ([viewsNumber integerValue] >= [viewsLimit integerValue])
      continue;
    
    NSDate * fireDate = [NSDateFormatter dateWithString:notificationInfo[@"NotificationDate"]];
    NSDate * expirationDate = [NSDateFormatter dateWithString:notificationInfo[@"NotificationExpirationDate"]];
    NSDate * currentDate = [NSDate date];
    if (expirationDate && [currentDate timeIntervalSinceDate:expirationDate] >= 0)
      continue;
    
    if ([currentDate timeIntervalSinceDate:fireDate] >= 0)
      fireDate = [NSDate dateWithTimeIntervalSinceNow:10.0 * 60];
    
    [self increaseViewsNumberOfNotification:notificationID];
    
    UILocalNotification * notification = [[UILocalNotification alloc] init];
    notification.alertBody = L(notificationInfo[@"NotificationLocalizedBodyKey"]);
    notification.fireDate = fireDate;
    notification.soundName = UILocalNotificationDefaultSoundName;
    notification.alertAction = L(notificationInfo[@"NotificationActionTitleKey"]);
    notification.userInfo = @{LocalNotificationManagerSpecialNotificationInfoKey : notificationInfo};
    
    UIApplication * application = [UIApplication sharedApplication];
    [application scheduleLocalNotification:notification];
    [actualSpecialLocalNotifications addObject:notification];
    
    [[Statistics instance] logEvent:[NSString stringWithFormat:@"'%@' Notification Scheduled", notificationID]];
  }
  
  // We'd like to remove not actual special notifications.
  NSMutableArray * notActualSpecialLocalNotifications = [[self scheduledSpecialLocalNotifications] mutableCopy];
  [notActualSpecialLocalNotifications removeObjectsInArray:actualSpecialLocalNotifications];
  for (UILocalNotification * notification in notActualSpecialLocalNotifications)
    [[UIApplication sharedApplication] cancelLocalNotification:notification];
}

- (NSArray *)localNotificationsInfo
{
  NSString * localNotificationsInfoFileName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"LocalNotificationsFileName"];
  NSString * localNotificationsInfoFilePath = [[NSBundle mainBundle] pathForResource:localNotificationsInfoFileName ofType:@"plist"];
  NSArray * localNotificationsInfo = [NSArray arrayWithContentsOfFile:localNotificationsInfoFilePath];
  return localNotificationsInfo;
}

#pragma mark - Location Notifications

- (void)showDownloadMapNotificationIfNeeded:(void (^)(UIBackgroundFetchResult))completionHandler
{
  self.downloadMapCompletionHandler = completionHandler;
  self.timer = [NSTimer scheduledTimerWithTimeInterval:25 target:self selector:@selector(timerSelector:) userInfo:nil repeats:NO];
  if ([CLLocationManager locationServicesEnabled])
    [self.locationManager startUpdatingLocation];
  else
    completionHandler(UIBackgroundFetchResultFailed);
}

- (void)markNotificationShowingForIndex:(TIndex)index
{
  NSMutableDictionary * flags = [[[NSUserDefaults standardUserDefaults] objectForKey:FLAGS_KEY] mutableCopy];
  if (!flags)
    flags = [[NSMutableDictionary alloc] init];
  
  flags[[self flagStringForIndex:index]] = [NSDate date];
  
  NSUserDefaults * userDefaults = [NSUserDefaults standardUserDefaults];
  [userDefaults setObject:flags forKey:FLAGS_KEY];
  [userDefaults synchronize];
}

- (BOOL)shouldShowNotificationForIndex:(TIndex)index
{
  NSDictionary * flags = [[NSUserDefaults standardUserDefaults] objectForKey:FLAGS_KEY];
  NSDate * lastShowDate = flags[[self flagStringForIndex:index]];
  return !lastShowDate || [[NSDate date] timeIntervalSinceDate:lastShowDate] > SHOW_INTERVAL;
}

- (void)timerSelector:(id)sender
{
  // Location still was not received but it's time to finish up so system will not kill us.
  [self.locationManager stopUpdatingLocation];
  self.downloadMapCompletionHandler(UIBackgroundFetchResultFailed);
}

- (void)downloadCountryWithIndex:(TIndex)index
{
  /// @todo Fix this logic after Framework -> CountryTree -> ActiveMapLayout refactoring.
  /// Call download via Framework.

  Framework & f = GetFramework();
  f.GetCountryTree().GetActiveMapLayout().DownloadMap(index, TMapOptions::EMap);
  m2::RectD const rect = f.GetCountryBounds(index);
  double const lon = MercatorBounds::XToLon(rect.Center().x);
  double const lat = MercatorBounds::YToLat(rect.Center().y);
  f.ShowRect(lat, lon, 10);
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

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
  [self.timer invalidate];
  [self.locationManager stopUpdatingLocation];
  NSString * flurryEventName = @"'Download Map' Notification Didn't Schedule";
  UIBackgroundFetchResult result = UIBackgroundFetchResultNoData;

  BOOL const inBackground = [UIApplication sharedApplication].applicationState == UIApplicationStateBackground;
  BOOL const onWiFi = [[AppInfo sharedInfo].reachability isReachableViaWiFi];
  if (inBackground && onWiFi)
  {
    Framework & f = GetFramework();
    CLLocation * lastLocation = [locations lastObject];
    TIndex const index = f.GetCountryIndex(MercatorBounds::FromLatLon(lastLocation.coordinate.latitude, lastLocation.coordinate.longitude));
    
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
        notification.userInfo = @{@"Action" : DOWNLOAD_MAP_ACTION_NAME, @"Group" : @(index.m_group), @"Country" : @(index.m_country), @"Region" : @(index.m_region)};
        
        UIApplication * application = [UIApplication sharedApplication];
        [application presentLocalNotificationNow:notification];

        [Alohalytics logEvent:@"suggestedToDownloadMissingMapForCurrentLocation" atLocation:lastLocation];
        flurryEventName = @"'Download Map' Notification Scheduled";
        result = UIBackgroundFetchResultNewData;
      }
    }
  }
  [[Statistics instance] logEvent:flurryEventName withParameters:@{@"WiFi" : @(onWiFi)}];
  self.downloadMapCompletionHandler(result);
}

@end
