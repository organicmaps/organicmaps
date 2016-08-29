#import "MWMHelpController.h"
#import <MessageUI/MFMailComposeViewController.h>
#import <sys/utsname.h>
#import "Common.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "WebViewController.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/platform.hpp"

extern NSString * const kAlohalyticsTapEventKey;
extern NSString * const kLocaleUsedInSupportEmails = @"en_gb";
extern NSDictionary * const kDeviceNames = @{
  @"i386" : @"Simulator",
  @"iPad1,1" : @"iPad WiFi",
  @"iPad1,2" : @"iPad GSM",
  @"iPad2,1" : @"iPad 2 WiFi",
  @"iPad2,2" : @"iPad 2 CDMA",
  @"iPad2,2" : @"iPad 2 GSM",
  @"iPad2,3" : @"iPad 2 GSM EV-DO",
  @"iPad2,4" : @"iPad 2",
  @"iPad2,5" : @"iPad Mini WiFi",
  @"iPad2,6" : @"iPad Mini GSM",
  @"iPad2,7" : @"iPad Mini CDMA",
  @"iPad3,1" : @"iPad 3rd gen. WiFi",
  @"iPad3,2" : @"iPad 3rd gen. GSM",
  @"iPad3,3" : @"iPad 3rd gen. CDMA",
  @"iPad3,4" : @"iPad 4th gen. WiFi",
  @"iPad3,5" : @"iPad 4th gen. GSM",
  @"iPad3,6" : @"iPad 4th gen. CDMA",
  @"iPad4,1" : @"iPad Air WiFi",
  @"iPad4,2" : @"iPad Air GSM",
  @"iPad4,3" : @"iPad Air CDMA",
  @"iPad4,4" : @"iPad Mini 2nd gen. WiFi",
  @"iPad4,5" : @"iPad Mini 2nd gen. GSM",
  @"iPad4,6" : @"iPad Mini 2nd gen. CDMA",
  @"iPad5,3" : @"iPad Air 2 WiFi",
  @"iPad5,4" : @"iPad Air 2 GSM",
  @"iPad6,3" : @"iPad Pro (9.7 inch) WiFi",
  @"iPad6,4" : @"iPad Pro (9.7 inch) GSM",
  @"iPad6,7" : @"iPad Pro (12.9 inch) WiFi",
  @"iPad6,8" : @"iPad Pro (12.9 inch) GSM",
  @"iPhone1,1" : @"iPhone",
  @"iPhone1,2" : @"iPhone 3G",
  @"iPhone2,1" : @"iPhone 3GS",
  @"iPhone3,1" : @"iPhone 4 GSM",
  @"iPhone3,2" : @"iPhone 4 CDMA",
  @"iPhone3,3" : @"iPhone 4 GSM EV-DO",
  @"iPhone4,1" : @"iPhone 4S",
  @"iPhone4,2" : @"iPhone 4S",
  @"iPhone4,3" : @"iPhone 4S",
  @"iPhone5,1" : @"iPhone 5",
  @"iPhone5,2" : @"iPhone 5",
  @"iPhone5,3" : @"iPhone 5c",
  @"iPhone5,4" : @"iPhone 5c",
  @"iPhone6,1" : @"iPhone 5s",
  @"iPhone6,2" : @"iPhone 5s",
  @"iPhone7,1" : @"iPhone 6 Plus",
  @"iPhone7,2" : @"iPhone 6",
  @"iPhone8,1" : @"iPhone 6s",
  @"iPhone8,2" : @"iPhone 6s Plus",
  @"iPhone8,4" : @"iPhone SE",
  @"iPod1,1" : @"iPod Touch",
  @"iPod2,1" : @"iPod Touch 2nd gen.",
  @"iPod3,1" : @"iPod Touch 3rd gen.",
  @"iPod4,1" : @"iPod Touch 4th gen.",
  @"iPod5,1" : @"iPod Touch 5th gen.",
  @"x86_64" : @"Simulator",
};

namespace
{
NSString * const kCommonReportActionTitle = L(@"leave_a_review");
NSString * const kBugReportActionTitle = L(@"something_is_not_working");
NSString * const kCancelActionTitle = L(@"cancel");
NSString * const kiOSEmail = @"ios@maps.me";
}

@interface MWMHelpController ()<UIActionSheetDelegate, MFMailComposeViewControllerDelegate>

@property(nonatomic) WebViewController * aboutViewController;

@property(weak, nonatomic) IBOutlet UIView * separatorView;

@end  // namespace

@implementation MWMHelpController

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = L(@"help");

  NSString * html;
  if (GetPlatform().ConnectionStatus() == Platform::EConnectionType::CONNECTION_NONE)
  {
    NSString * path = [[NSBundle mainBundle] pathForResource:@"faq" ofType:@"html"];
    html = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    self.aboutViewController =
        [[WebViewController alloc] initWithHtml:html baseUrl:nil andTitleOrNil:nil];
  }
  else
  {
    NSURL * url = [NSURL URLWithString:@"https://support.maps.me"];
    self.aboutViewController = [[WebViewController alloc] initWithUrl:url andTitleOrNil:nil];
  }

  self.aboutViewController.openInSafari = NO;
  UIView * aboutView = self.aboutViewController.view;
  [self addChildViewController:self.aboutViewController];
  [self.view addSubview:aboutView];

  aboutView.translatesAutoresizingMaskIntoConstraints = NO;
  NSLayoutConstraint * top = [NSLayoutConstraint constraintWithItem:self.view
                                                          attribute:NSLayoutAttributeTop
                                                          relatedBy:NSLayoutRelationEqual
                                                             toItem:aboutView
                                                          attribute:NSLayoutAttributeTop
                                                         multiplier:1.0
                                                           constant:0.0];
  NSLayoutConstraint * bottom = [NSLayoutConstraint constraintWithItem:self.separatorView
                                                             attribute:NSLayoutAttributeTop
                                                             relatedBy:NSLayoutRelationEqual
                                                                toItem:aboutView
                                                             attribute:NSLayoutAttributeBottom
                                                            multiplier:1.0
                                                              constant:0.0];
  NSLayoutConstraint * left = [NSLayoutConstraint constraintWithItem:self.view
                                                           attribute:NSLayoutAttributeLeft
                                                           relatedBy:NSLayoutRelationEqual
                                                              toItem:aboutView
                                                           attribute:NSLayoutAttributeLeft
                                                          multiplier:1.0
                                                            constant:0.0];
  NSLayoutConstraint * right = [NSLayoutConstraint constraintWithItem:self.view
                                                            attribute:NSLayoutAttributeRight
                                                            relatedBy:NSLayoutRelationEqual
                                                               toItem:aboutView
                                                            attribute:NSLayoutAttributeRight
                                                           multiplier:1.0
                                                             constant:0.0];

  [self.view addConstraints:@[ top, bottom, left, right ]];
}

- (IBAction)reportBug
{
  if (isIOS7)
    [self reportIOS7];
  else
    [self reportRegular];
}

- (void)reportIOS7
{
  UIActionSheet * actionSheet =
      [[UIActionSheet alloc] initWithTitle:nil
                                  delegate:self
                         cancelButtonTitle:kCancelActionTitle
                    destructiveButtonTitle:nil
                         otherButtonTitles:kCommonReportActionTitle, kBugReportActionTitle, nil];
  [actionSheet showInView:self.view];
}

- (void)reportRegular
{
  UIAlertController * alert =
      [UIAlertController alertControllerWithTitle:L(@"report_a_bug")
                                          message:nil
                                   preferredStyle:UIAlertControllerStyleAlert];

  UIAlertAction * commonReport = [UIAlertAction actionWithTitle:kCommonReportActionTitle
                                                          style:UIAlertActionStyleDefault
                                                        handler:^(UIAlertAction * _Nonnull action) {
                                                          [self commonReportAction];
                                                        }];
  [alert addAction:commonReport];

  UIAlertAction * bugReport = [UIAlertAction actionWithTitle:kBugReportActionTitle
                                                       style:UIAlertActionStyleDefault
                                                     handler:^(UIAlertAction * _Nonnull action) {
                                                       [self bugReportAction];
                                                     }];
  [alert addAction:bugReport];

  UIAlertAction * cancel =
      [UIAlertAction actionWithTitle:kCancelActionTitle style:UIAlertActionStyleCancel handler:nil];
  [alert addAction:cancel];
  alert.preferredAction = cancel;

  [self presentViewController:alert animated:YES completion:nil];
}

#pragma mark - Actions

- (void)commonReportAction
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"contactUs"];
  [self sendEmailWithText:nil subject:@"MAPS.ME" toRecipient:kiOSEmail];
}

- (void)bugReportAction
{
  [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatReport}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"reportABug"];
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString * machine =
      [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
  NSString * device = kDeviceNames[machine];
  if (!device)
    device = machine;
  NSString * languageCode = [[NSLocale preferredLanguages] firstObject];
  NSString * language = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails]
      displayNameForKey:NSLocaleLanguageCode
                  value:languageCode];
  NSString * locale = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
  NSString * country = [[NSLocale localeWithLocaleIdentifier:kLocaleUsedInSupportEmails]
      displayNameForKey:NSLocaleCountryCode
                  value:locale];
  NSString * bundleVersion =
      [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- MAPS.ME %@\n- %@/%@", device,
                                               [UIDevice currentDevice].systemVersion,
                                               bundleVersion, language, country];
  NSString * alohalyticsId = [Alohalytics installationId];
  if (alohalyticsId)
    text = [NSString stringWithFormat:@"%@\n- %@", text, alohalyticsId];
  [self sendEmailWithText:text subject:@"MAPS.ME" toRecipient:kiOSEmail];
}

#pragma mark - Email

- (void)sendEmailWithText:(NSString *)text subject:(NSString *)subject toRecipient:(NSString *)email
{
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [[MFMailComposeViewController alloc] init];
    vc.mailComposeDelegate = self;
    [vc setSubject:subject];
    [vc setMessageBody:text isHTML:NO];
    [vc setToRecipients:@[ email ]];
    [vc.navigationBar setTintColor:[UIColor whitePrimaryText]];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
    NSString * text = [NSString stringWithFormat:L(@"email_error_body"), email];
    [[[UIAlertView alloc] initWithTitle:L(@"email_error_title")
                                message:text
                               delegate:nil
                      cancelButtonTitle:L(@"ok")
                      otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller
          didFinishWithResult:(MFMailComposeResult)result
                        error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - UIActionSheetDelegate

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (actionSheet.numberOfButtons == 0 || buttonIndex >= actionSheet.numberOfButtons ||
      buttonIndex < 0)
  {
    [actionSheet dismissWithClickedButtonIndex:0 animated:NO];
    return;
  }
  NSString * btnTitle = [actionSheet buttonTitleAtIndex:buttonIndex];
  if ([btnTitle isEqualToString:kCommonReportActionTitle])
    [self commonReportAction];
  else if ([btnTitle isEqualToString:kBugReportActionTitle])
    [self bugReportAction];
}

@end
