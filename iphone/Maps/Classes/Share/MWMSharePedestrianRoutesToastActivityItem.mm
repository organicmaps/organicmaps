#import "Macros.h"
#import "MWMSharePedestrianRoutesToastActivityItem.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/preferred_languages.hpp"

@implementation MWMSharePedestrianRoutesToastActivityItem

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return NSString.string;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  NSString * event = @"MWMSharePedestrianRoutesToastActivityItem:activityViewController:itemForActivityType:";
  [Statistics logEvent:kStatEventName(kStatShare, kStatSocial) withParameters:@{kStatAction : activityType}];
  [Alohalytics logEvent:event withValue:activityType];
  if ([activityType isEqualToString:UIActivityTypePostToFacebook] ||
      [activityType isEqualToString:@"com.facebook.Facebook.ShareExtension"] ||
      [activityType.lowercaseString rangeOfString:@"facebook"].length)
  {
    NSString * url = [NSString stringWithFormat:@"http://maps.me/fb-pedestrian?lang=%@",
                      @(languages::GetCurrentNorm().c_str())];
    return [NSURL URLWithString:url];
  }
  if ([UIActivityTypeMessage isEqualToString:activityType])
    return L(@"share_walking_routes_sms");
  if ([UIActivityTypeMail isEqualToString:activityType])
    return L(@"share_walking_routes_email_body");
  return L(@"share_walking_routes_messenger");
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return L(@"share_walking_routes_email_subject");
}

@end
