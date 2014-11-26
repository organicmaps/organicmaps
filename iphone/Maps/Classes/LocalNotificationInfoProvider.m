//
//  LocalNotificationInfoProvider.m
//  Maps
//
//  Created by Timur Bernikowich on 25/11/2014.
//  Copyright (c) 2014 MapsWithMe. All rights reserved.
//

#import "LocalNotificationInfoProvider.h"
#import "UIKitCategories.h"

@implementation LocalNotificationInfoProvider

- (instancetype)initWithDictionary:(NSDictionary *)info
{
  self = [super init];
  if (self)
    _info = info;
  return self;
}

#pragma mark - Activity Item Source

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return [NSString string];
}

- (id)activityViewController:(UIActivityViewController *)activityViewController itemForActivityType:(NSString *)activityType
{
  NSString * textToShare;
  if ([activityType isEqualToString:UIActivityTypeMail])
    textToShare = L(self.info[@"NotificationLocalizedShareEmailBodyKey"]);
  else
    textToShare = L(self.info[@"NotificationLocalizedShareTextKey"]);
  NSURL * link = [NSURL URLWithString:self.info[@"NotificationShareLink"]];
  if (link)
    textToShare = [textToShare stringByAppendingFormat:@" %@", [link absoluteString]];
  return textToShare;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController subjectForActivityType:(NSString *)activityType
{
  NSString * emailSubject = L(self.info[@"NotificationLocalizedShareEmailSubjectKey"]);
  return emailSubject;
}

@end