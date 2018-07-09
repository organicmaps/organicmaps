#import "MWMCustomFacebookEvents.h"
#import "3party/Alohalytics/src/alohalytics_objc.h"

#import <FBSDKCoreKit/FBSDKCoreKit.h>

#include "Framework.h"

@implementation MWMCustomFacebookEvents

// Used to filter out old users and to track only new installs.
static NSString * const kEnableCustomFBEventsForNewUsers = @"FBEnableCustomEventsForNewUsers";
// Special one-time events to improve marketing targeting.
// NOTE: Event names are using some default FB names by Alexander Bobko's request.
static NSString * const kFirstSessionIsLongerThanXMinutesEvent = FBSDKAppEventNameAchievedLevel;
static NSInteger const kFirstSessionLengthInSeconds = 5 * 60;
static NSString * const kNextLaunchAfterHoursInterval = FBSDKAppEventNameCompletedRegistration;
static NSInteger const kNextLaunchMinHoursInterval = 6;
static NSString * const kDownloadedSecondMapEvent = FBSDKAppEventNameUnlockedAchievement;

static constexpr int kNotSubscribed = -1;
static int gStorageSubscriptionId = kNotSubscribed;

+ (void)markEventAsAlreadyFired:(NSString *)event
{
  NSUserDefaults * defaults = NSUserDefaults.standardUserDefaults;
  [defaults setBool:YES forKey:event];
  [defaults synchronize];
}

// This notification is called exactly once upon a whole life time of the app.
+ (void)applicationDidEnterBackgroundOnlyOnceInAnAppLifeTimeAtTheEndOfVeryFirstSession:(NSNotification *)notification
{
  [NSNotificationCenter.defaultCenter removeObserver:[MWMCustomFacebookEvents class]];
  NSInteger const seconds = [Alohalytics totalSecondsSpentInTheApp];
  if (seconds >= kFirstSessionLengthInSeconds)
    [FBSDKAppEvents logEvent:kFirstSessionIsLongerThanXMinutesEvent
                  parameters:@{FBSDKAppEventParameterNameLevel : [NSNumber numberWithInteger:(seconds / 60)]}];
  [MWMCustomFacebookEvents markEventAsAlreadyFired:kFirstSessionIsLongerThanXMinutesEvent];
}

+ (void)optimizeExpenses
{
  NSUserDefaults * defaults = NSUserDefaults.standardUserDefaults;
  BOOL const isFirstSession = [Alohalytics isFirstSession];
  if (isFirstSession)
  {
    [defaults setBool:YES forKey:kEnableCustomFBEventsForNewUsers];
    [defaults synchronize];
  }
  // All these events should be fired only for new users who installed the app.
  if ([defaults boolForKey:kEnableCustomFBEventsForNewUsers])
  {
    // Skip already fired events.
    // This one is fired when user spent more than kFirstSessionLengthInSeconds in the first session.
    if (isFirstSession && ![defaults boolForKey:kFirstSessionIsLongerThanXMinutesEvent])
    {
      [NSNotificationCenter.defaultCenter
          addObserver:[MWMCustomFacebookEvents class]
             selector:
                 @selector(
                     applicationDidEnterBackgroundOnlyOnceInAnAppLifeTimeAtTheEndOfVeryFirstSession:
                         )
                 name:UIApplicationDidEnterBackgroundNotification
               object:nil];
      [MWMCustomFacebookEvents markEventAsAlreadyFired:kFirstSessionIsLongerThanXMinutesEvent];
    }
    // This event is fired when user launched app again in between kNextLaunchMinHoursInterval and kNextLaunchMaxHoursInterval.
    if (![defaults boolForKey:kNextLaunchAfterHoursInterval])
    {
      NSInteger const hoursFromFirstLaunch =
          (-[Alohalytics firstLaunchDate].timeIntervalSinceNow) / 3600;
      if (hoursFromFirstLaunch >= kNextLaunchMinHoursInterval)
      {
        [FBSDKAppEvents logEvent:kNextLaunchAfterHoursInterval];
        [MWMCustomFacebookEvents markEventAsAlreadyFired:kNextLaunchAfterHoursInterval];
      }
    }
    // Fired when user downloads second (or more) map.
    if (![defaults boolForKey:kDownloadedSecondMapEvent])
    {
      if (gStorageSubscriptionId == kNotSubscribed)
      {
        gStorageSubscriptionId = GetFramework().GetStorage().Subscribe([](storage::TCountryId const &)
        {
          if (GetFramework().GetStorage().GetDownloadedFilesCount() >= 2)
          {
            [FBSDKAppEvents logEvent:kDownloadedSecondMapEvent];
            [MWMCustomFacebookEvents markEventAsAlreadyFired:kDownloadedSecondMapEvent];
            // We can't unsubscribe from this callback immediately now, it will crash Storage's observers notification.
            dispatch_async(dispatch_get_main_queue(),
            ^{
              GetFramework().GetStorage().Unsubscribe(gStorageSubscriptionId);
              gStorageSubscriptionId = kNotSubscribed;
            });
            [Alohalytics logEvent:kDownloadedSecondMapEvent];
          }
        }, [](storage::TCountryId const &, storage::MapFilesDownloader::TProgress const &){});
      }
    }
  }
}

@end
