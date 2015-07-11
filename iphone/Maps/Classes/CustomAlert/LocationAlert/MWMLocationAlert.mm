//
//  MWMLocationAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 10.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLocationAlert.h"
#import "MWMAlertViewController.h"

static NSString * const kLocationAlertNibName = @"MWMLocationAlert";

@implementation MWMLocationAlert

+ (instancetype)alert
{
  MWMLocationAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kLocationAlertNibName owner:nil options:nil] firstObject];
  [alert setNeedsCloseAlertAfterEnterBackground];
  return alert;
}

- (IBAction)settingsTap
{
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:UIApplicationOpenSettingsURLString]];
  [self.alertController closeAlert];
}

- (IBAction)closeTap
{
  [self.alertController closeAlert];
}

@end
