#import "LocalNotificationManager.h"
#import "CoreNotificationWrapper+Core.h"
#import "NSDate+TimeDistance.h"
#import "Statistics.h"

#include "map/framework_light.hpp"
#include "map/framework_light_delegate.hpp"

#include "platform/network_policy_ios.h"

#include <CoreApi/Framework.h>

static NSString * const kLastUGCNotificationDate = @"LastUGCNotificationDate";

@implementation LocalNotificationManager

+ (BOOL)shouldShowAuthNotification
{
  if (!network_policy::CanUseNetwork())
    return NO;

  if (NSDate * date = [[NSUserDefaults standardUserDefaults] objectForKey:kLastUGCNotificationDate])
  {
    if (date.daysToNow <= 5)
      return NO;
  }

  using namespace lightweight;
  lightweight::Framework const f(REQUEST_TYPE_NUMBER_OF_UNSENT_UGC | REQUEST_TYPE_USER_AUTH_STATUS);
  if (f.IsUserAuthenticated() || f.GetNumberOfUnsentUGC() < 2)
    return NO;

  return YES;
}

+ (void)authNotificationWasShown
{
  auto ud = [NSUserDefaults standardUserDefaults];
  [ud setObject:[NSDate date] forKey:kLastUGCNotificationDate];
  [ud synchronize];
  [Statistics logEvent:@"UGC_UnsentNotification_shown"];
}

+ (CoreNotificationWrapper *)reviewNotificationWrapper
{
  lightweight::Framework framework(lightweight::REQUEST_TYPE_NOTIFICATION);
  framework.SetDelegate(std::make_unique<FrameworkLightDelegate>(GetFramework()));
  auto const notificationCandidate = framework.GetNotification();

  if (notificationCandidate)
  {
    auto const notification = *notificationCandidate;
    if (notification.GetType() == notifications::NotificationCandidate::Type::UgcReview)
    {
      CoreNotificationWrapper * w = [[CoreNotificationWrapper alloc] initWithNotificationCandidate:notification];
      return w;
    }
  }

  return nil;
}

+ (void)reviewNotificationWasShown
{
  [Statistics logEvent:kStatUGCReviewNotificationShown];
}

@end
