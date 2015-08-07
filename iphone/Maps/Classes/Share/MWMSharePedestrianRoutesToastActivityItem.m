//
//  MWMSharePedestrianRoutesToastActivityItem.m
//  Maps
//
//  Created by Ilya Grechuhin on 06.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MWMSharePedestrianRoutesToastActivityItem.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

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
  [Alohalytics logEvent:event withValue:activityType];
  if ([UIActivityTypePostToFacebook isEqualToString:activityType])
    return [NSURL URLWithString:@"http://maps.me/fb_share"];
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
