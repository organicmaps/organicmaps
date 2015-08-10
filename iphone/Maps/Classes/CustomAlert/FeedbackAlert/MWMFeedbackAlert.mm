//
//  MWMFeedbackAlert.m
//  Maps
//
//  Created by v.mikhaylenko on 25.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "AppInfo.h"
#import "MWMAlertViewController.h"
#import "MWMFeedbackAlert.h"
#import "Statistics.h"
#import "UIKitCategories.h"

#import <MessageUI/MFMailComposeViewController.h>
#import <sys/utsname.h>

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/platform.hpp"

@interface MWMFeedbackAlert () <MFMailComposeViewControllerDelegate>

@property (nonatomic, assign) NSUInteger starsCount;

@end

static NSString * const kFeedbackAlertNibName = @"MWMFeedbackAlert";
static NSString * const kRateEmail = @"rating@maps.me";
extern NSDictionary * const deviceNames;
extern NSString * const kLocaleUsedInSupportEmails;
extern NSString * const kRateAlertEventName;

@implementation MWMFeedbackAlert

+ (instancetype)alertWithStarsCount:(NSUInteger)starsCount
{
  [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kRateAlertEventName, @"open"]];
  MWMFeedbackAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kFeedbackAlertNibName owner:self options:nil] firstObject];
  alert.starsCount = starsCount;
  return alert;
}

#pragma mark - Actions

- (IBAction)notNowButtonTap:(id)sender
{
  [Alohalytics logEvent:kRateAlertEventName withValue:@"feedbackNotNowTap"];
  [self close];
}

- (IBAction)writeFeedbackButtonTap:(id)sender
{
  [Alohalytics logEvent:kRateAlertEventName withValue:@"feedbackWriteTap"];
  [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kRateAlertEventName, @"feedbackWriteTap"]];
  self.alpha = 0.;
  self.alertController.view.alpha = 0.;
  if ([MFMailComposeViewController canSendMail])
  {
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString * machine = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    NSString * device = deviceNames[machine];
    if (!device)
      device = machine;
    NSString * languageCode = [[NSLocale preferredLanguages] firstObject];
    NSString * language = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails] displayNameForKey:NSLocaleLanguageCode value:languageCode];
    NSString * locale = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
    NSString * country = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails] displayNameForKey:NSLocaleCountryCode value:locale];
    NSString * bundleVersion = AppInfo.sharedInfo.bundleVersion;
    NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- MAPS.ME %@\n- %@/%@", device, [UIDevice currentDevice].systemVersion, bundleVersion, language, country];
    MFMailComposeViewController * mailController = [[MFMailComposeViewController alloc] init];
    mailController.mailComposeDelegate = self;
    [mailController setSubject:[NSString stringWithFormat:@"%@ : %@", L(@"rating_just_rated"), @(self.starsCount)]];
    [mailController setToRecipients:@[kRateEmail]];
    [mailController setMessageBody:text isHTML:NO];
    mailController.navigationBar.tintColor = [UIColor blackColor];
    [self.alertController.ownerViewController presentViewController:mailController animated:YES completion:nil];
  }
  else
  {
    NSString * text = [NSString stringWithFormat:L(@"email_error_body"), kRateEmail];
    [[[UIAlertView alloc] initWithTitle:L(@"email_error_title") message:text delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
  }
}

#pragma mark - MFMailComposeViewControllerDelegate

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error {
  [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kRateAlertEventName, @"mailComposeController"]];
  [self.alertController.ownerViewController dismissViewControllerAnimated:YES completion:^
  {
    [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kRateAlertEventName, @"close"]];
    [self close];
  }];
}

@end
