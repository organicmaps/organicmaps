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
  [Alohalytics logEvent:event withValue:activityType];
  [Statistics.instance logEvent:event withParameters:@{@"type" : activityType}];
  if ([UIActivityTypePostToFacebook isEqualToString:activityType])
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
