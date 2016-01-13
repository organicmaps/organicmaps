#import "CommunityVC.h"
#import "RichTextVC.h"
#import "SettingsAndMoreVC.h"
#import "SettingsViewController.h"
#import "Statistics.h"
#import "UIViewController+Navigation.h"
#import "WebViewController.h"
#import <MessageUI/MFMailComposeViewController.h>
#import <sys/utsname.h>

#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "platform/platform.hpp"

@interface SettingsAndMoreVC () <MFMailComposeViewControllerDelegate>

@property (nonatomic) NSArray * items;

@end

static NSString * const kiOSEmail = @"ios@maps.me";
extern NSString * const kLocaleUsedInSupportEmails = @"en_gb";
extern NSString * const kAlohalyticsTapEventKey;
extern NSDictionary * const deviceNames = @{@"x86_64" : @"Simulator",
                                     @"i386" : @"Simulator",
                                     @"iPod1,1" : @"iPod Touch",
                                     @"iPod2,1" : @"iPod Touch 2nd gen.",
                                     @"iPod3,1" : @"iPod Touch 3rd gen.",
                                     @"iPod4,1" : @"iPod Touch 4th gen.",
                                     @"iPod5,1" : @"iPod Touch 5th gen.",
                                     @"iPhone1,1" : @"iPhone",
                                     @"iPhone1,2" : @"iPhone 3G",
                                     @"iPhone2,1" : @"iPhone 3GS",
                                     @"iPhone3,1" : @"iPhone 4",
                                     @"iPhone4,1" : @"iPhone 4S",
                                     @"iPhone4,2" : @"iPhone 4S",
                                     @"iPhone4,3" : @"iPhone 4S",
                                     @"iPhone5,1" : @"iPhone 5",
                                     @"iPhone5,2" : @"iPhone 5",
                                     @"iPhone5,3" : @"iPhone 5c",
                                     @"iPhone5,4" : @"iPhone 5c",
                                     @"iPhone6,1" : @"iPhone 5s",
                                     @"iPhone6,2" : @"iPhone 5s",
                                     @"iPad1,1" : @"iPad WiFi",
                                     @"iPad1,2" : @"iPad GSM",
                                     @"iPad2,1" : @"iPad 2 WiFi",
                                     @"iPad2,2" : @"iPad 2 GSM",
                                     @"iPad2,2" : @"iPad 2 CDMA",
                                     @"iPad3,1" : @"iPad 3rd gen. WiFi",
                                     @"iPad3,2" : @"iPad 3rd gen. GSM",
                                     @"iPad3,3" : @"iPad 3rd gen. CDMA",
                                     @"iPad3,4" : @"iPad 4th gen. WiFi",
                                     @"iPad3,5" : @"iPad 4th gen. GSM",
                                     @"iPad3,6" : @"iPad 4th gen. CDMA",
                                     @"iPad4,1" : @"iPad Air WiFi",
                                     @"iPad4,2" : @"iPad Air GSM",
                                     @"iPad4,3" : @"iPad Air CDMA",
                                     @"iPad2,5" : @"iPad Mini WiFi",
                                     @"iPad2,6" : @"iPad Mini GSM",
                                     @"iPad2,7" : @"iPad Mini CDMA",
                                     @"iPad4,4" : @"iPad Mini 2nd gen. WiFi",
                                     @"iPad4,5" : @"iPad Mini 2nd gen. GSM",
                                     @"iPad4,6" : @"iPad Mini 2nd gen. CDMA"};


@implementation SettingsAndMoreVC

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"settings_and_more");
  self.items = @[@{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Settings", @"Title" : L(@"settings"), @"Icon" : @"ic_settings_settings"},
                                @{@"Id" : @"Help", @"Title" : L(@"help"), @"Icon" : @"ic_settings_help"},
                                @{@"Id" : @"ReportBug", @"Title" : L(@"report_a_bug"), @"Icon" : @"ic_settings_feedback"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Community", @"Title" : L(@"maps_me_community"), @"Icon" : @"ic_settings_community"},
                                @{@"Id" : @"RateApp", @"Title" : L(@"rate_the_app"), @"Icon" : @"ic_settings_rate"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"About", @"Title" : L(@"about_menu_title"), @"Icon" : @"IconAbout"},
                                @{@"Id" : @"Copyright", @"Title" : L(@"copyright"), @"Icon" : @"IconCopyright"}]}];
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [self.items count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return 0.001;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return 20;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return self.items[section][@"Title"];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self.items[section][@"Items"] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSDictionary * item = self.items[indexPath.section][@"Items"][indexPath.row];

  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  if (!cell) // iOS 5
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[UITableViewCell className]];

  cell.backgroundColor = [UIColor white];
  cell.textLabel.text = item[@"Title"];
  cell.imageView.image = [UIImage imageNamed:item[@"Icon"]];
  cell.imageView.mwm_coloring = MWMImageColoringBlack;
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  cell.textLabel.backgroundColor = [UIColor clearColor];

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  NSString * itemId = self.items[indexPath.section][@"Items"][indexPath.row][@"Id"];
  if ([itemId isEqualToString:@"About"])
    [self about];
  else if ([itemId isEqualToString:@"Community"])
    [self community];
  else if ([itemId isEqualToString:@"RateApp"])
    [self rateApp];
  else if ([itemId isEqualToString:@"Settings"])
    [self settings];
  else if ([itemId isEqualToString:@"ReportBug"])
    [self reportBug];
  else if ([itemId isEqualToString:@"Help"])
    [self help];
  else if ([itemId isEqualToString:@"Copyright"])
    [self copyright];
}

- (void)settings
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatSettings}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsMiles"];
  SettingsViewController * vc = [self.mainStoryboard instantiateViewControllerWithIdentifier:[SettingsViewController className]];
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)community
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatSocial}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"community"];
  CommunityVC * vc = [[CommunityVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)help
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatHelp}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"help"];
  NSString * path = [[NSBundle mainBundle] pathForResource:@"faq" ofType:@"html"];
  NSString * html = [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:html baseUrl:nil andTitleOrNil:L(@"help")];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)about
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatAbout}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"about"];
  RichTextVC * vc = [[RichTextVC alloc] initWithText:L(@"about_text")];
  vc.title = L(@"about_menu_title");
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)copyright
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatCopyright}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"copyright"];
  string s; GetPlatform().GetReader("copyright.html")->ReadAsString(s);
  NSString * str = [NSString stringWithFormat:@"Version: %@ \n", [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"]];
  NSString * text = [NSString stringWithFormat:@"%@%@", str, @(s.c_str())];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:L(@"copyright")];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)rateApp
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatRate}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"rate"];
  [[UIApplication sharedApplication] rateVersionFrom:@"rate_menu_item"];
}

- (void)reportBug
{
  [[Statistics instance] logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatReport}];
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"reportABug"];
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
  NSString * bundleVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- MAPS.ME %@\n- %@/%@", device, [UIDevice currentDevice].systemVersion, bundleVersion, language, country];
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [[MFMailComposeViewController alloc] init];
    vc.mailComposeDelegate = self;
    [vc setSubject:@"MAPS.ME"];
    [vc setToRecipients:@[kiOSEmail]];
    [vc setMessageBody:text isHTML:NO];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
    NSString * text = [NSString stringWithFormat:L(@"email_error_body"), kiOSEmail];
    [[[UIAlertView alloc] initWithTitle:L(@"email_error_title") message:text delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

@end
