
#import "SettingsAndMoreVC.h"
#import "UIKitCategories.h"
#import <MessageUI/MFMailComposeViewController.h>
#import <sys/utsname.h>
#include "../../platform/platform.hpp"
#import "SettingsViewController.h"
#import "UIViewController+Navigation.h"
#import "WebViewController.h"
#import "CommunityVC.h"
#import "RichTextVC.h"
#import "../../3party/Alohalytics/src/alohalytics_objc.h"


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
                   @"Items" : @[@{@"Id" : @"Settings", @"Title" : L(@"settings"), @"Icon" : @"IconAppSettings"},
                                @{@"Id" : @"Help", @"Title" : L(@"help"), @"Icon" : @"IconHelp"},
                                @{@"Id" : @"ReportBug", @"Title" : L(@"report_a_bug"), @"Icon" : @"IconReportABug"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Community", @"Title" : L(@"maps_me_community"), @"Icon" : @"IconSocial"},
                                @{@"Id" : @"RateApp", @"Title" : L(@"rate_the_app"), @"Icon" : @"IconRate"}]},
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

  cell.textLabel.text = item[@"Title"];
  cell.imageView.image = [UIImage imageNamed:item[@"Icon"]];

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * itemId = self.items[indexPath.section][@"Items"][indexPath.row][@"Id"];
  if ([itemId isEqualToString:@"About"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"about"];
    [self about];
  }
  else if ([itemId isEqualToString:@"Community"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"community"];
    CommunityVC * vc = [[CommunityVC alloc] initWithStyle:UITableViewStyleGrouped];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else if ([itemId isEqualToString:@"RateApp"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"rate"];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self rateApp];
  }
  else if ([itemId isEqualToString:@"Settings"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsMiles"];
    SettingsViewController * vc = [self.mainStoryboard instantiateViewControllerWithIdentifier:[SettingsViewController className]];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else if ([itemId isEqualToString:@"ReportBug"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"reportABug"];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self reportBug];
  }
  else if ([itemId isEqualToString:@"Help"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"help"];
    [self help];
  }
  else if ([itemId isEqualToString:@"Copyright"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"copyright"];
    [self copyright];
  }
}

- (void)help
{
  NSString * path = [[NSBundle mainBundle] pathForResource:@"faq" ofType:@"html"];
  NSString * html = [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:html baseUrl:nil andTitleOrNil:L(@"help")];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)about
{
  RichTextVC * vc = [[RichTextVC alloc] initWithText:L(@"about_text")];
  vc.title = L(@"about_menu_title");
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)copyright
{
  string s; GetPlatform().GetReader("copyright.html")->ReadAsString(s);
  NSString * str = [NSString stringWithFormat:@"Version: %@ \n", [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"]];
  NSString * text = [NSString stringWithFormat:@"%@%@", str, [NSString stringWithUTF8String:s.c_str()]];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:L(@"copyright")];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)rateApp
{
  [[UIApplication sharedApplication] rateVersionFrom:@"rate_menu_item"];
}

- (void)reportBug
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
