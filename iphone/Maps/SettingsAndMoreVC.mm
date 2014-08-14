
#import "SettingsAndMoreVC.h"
#import "UIKitCategories.h"
#import <MessageUI/MFMailComposeViewController.h>
#import <sys/utsname.h>
#include "../../map/dialog_settings.hpp"
#include "../../platform/platform.hpp"
#import "SettingsViewController.h"
#import "UIViewController+Navigation.h"
#import "WebViewController.h"
#import "CommunityVC.h"
#import "RichTextVC.h"

@interface SettingsAndMoreVC () <MFMailComposeViewControllerDelegate>

@property (nonatomic) NSArray * items;
@property (nonatomic) NSDictionary * deviceNames;

@end

@implementation SettingsAndMoreVC

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = NSLocalizedString(@"settings_and_more", nil);

  self.deviceNames = @{@"x86_64" : @"Simulator",
                       @"i386" : @"Simulator",
                       @"iPod1,1" : @"iPod Touch",
                       @"iPod2,1" : @"iPod Touch 2nd generation",
                       @"iPod3,1" : @"iPod Touch 3rd generation",
                       @"iPod4,1" : @"iPod Touch 4th generation",
                       @"iPhone1,1" : @"iPhone",
                       @"iPhone1,2" : @"iPhone 3G",
                       @"iPhone2,1" : @"iPhone 3GS",
                       @"iPhone3,1" : @"iPhone 4",
                       @"iPhone4,1" : @"iPhone 4S",
                       @"iPhone5,1" : @"iPhone 5",
                       @"iPhone5,2" : @"iPhone 5",
                       @"iPhone5,3" : @"iPhone 5c",
                       @"iPhone5,4" : @"iPhone 5c",
                       @"iPhone6,1" : @"iPhone 5s",
                       @"iPhone6,2" : @"iPhone 5s",
                       @"iPad1,1" : @"iPad",
                       @"iPad2,1" : @"iPad 2",
                       @"iPad3,1" : @"iPad 3rd generation",
                       @"iPad3,4" : @"iPad 4th generation",
                       @"iPad2,5" : @"iPad Mini",
                       @"iPad4,1" : @"iPad Air - Wifi",
                       @"iPad4,2" : @"iPad Air - Cellular",
                       @"iPad4,4" : @"iPad Mini - Wifi 2nd generation",
                       @"iPad4,5" : @"iPad Mini - Cellular 2nd generation"};

  self.items = @[@{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Settings", @"Title" : NSLocalizedString(@"settings", nil), @"Icon" : @"IconAppSettings"},
                                @{@"Id" : @"Help", @"Title" : NSLocalizedString(@"help", nil), @"Icon" : @"IconHelp"},
                                @{@"Id" : @"ReportBug", @"Title" : NSLocalizedString(@"report_a_bug", nil), @"Icon" : @"IconReportABug"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Community", @"Title" : NSLocalizedString(@"maps_me_community", nil), @"Icon" : @"IconSocial"},
                                @{@"Id" : @"RateApp", @"Title" : NSLocalizedString(@"rate_the_app", nil), @"Icon" : @"IconRate"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"About", @"Title" : NSLocalizedString(@"about_menu_title", nil), @"Icon" : @"IconAbout"},
                                @{@"Id" : @"Copyright", @"Title" : NSLocalizedString(@"copyright", nil), @"Icon" : @"IconCopyright"}]}];
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
    [self about];
  }
  else if ([itemId isEqualToString:@"Community"])
  {
    CommunityVC * vc = [[CommunityVC alloc] initWithStyle:UITableViewStyleGrouped];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else if ([itemId isEqualToString:@"RateApp"])
  {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self rateApp];
  }
  else if ([itemId isEqualToString:@"Settings"])
  {
    SettingsViewController * vc = [self.mainStoryboard instantiateViewControllerWithIdentifier:[SettingsViewController className]];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else if ([itemId isEqualToString:@"ReportBug"])
  {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self reportBug];
  }
  else if ([itemId isEqualToString:@"Help"])
  {
    [self help];
  }
  else if ([itemId isEqualToString:@"Copyright"])
  {
    [self copyright];
  }
}

- (void)help
{
  NSString * path = [[NSBundle mainBundle] pathForResource:@"faq" ofType:@"html"];
  NSString * html = [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:html baseUrl:nil andTitleOrNil:NSLocalizedString(@"help", nil)];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)about
{
  RichTextVC * vc = [[RichTextVC alloc] initWithText:NSLocalizedString(@"about_text", nil)];
  vc.title = NSLocalizedString(@"about_menu_title", nil);
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)copyright
{
  string s; GetPlatform().GetReader("copyright.html")->ReadAsString(s);
  NSString * str = [NSString stringWithFormat:@"Version: %@ \n", [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"]];
  NSString * text = [NSString stringWithFormat:@"%@%@", str, [NSString stringWithUTF8String:s.c_str()]];
  WebViewController * aboutViewController = [[WebViewController alloc] initWithHtml:text baseUrl:nil andTitleOrNil:NSLocalizedString(@"copyright", nil)];
  aboutViewController.openInSafari = YES;
  [self.navigationController pushViewController:aboutViewController animated:YES];
}

- (void)rateApp
{
  dlg_settings::SaveResult(dlg_settings::AppStore, dlg_settings::OK);
  if (GetPlatform().IsPro())
    [[UIApplication sharedApplication] rateProVersionFrom:@"ios_pro_popup"];
  else
    [[UIApplication sharedApplication] rateLiteVersionFrom:@"ios_lite_popup"];
}

- (void)reportBug
{
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString * machine = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
  NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- MAPS.ME %@", self.deviceNames[machine], [UIDevice currentDevice].systemVersion, [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey]];
  NSString * email = @"ios@maps.me";
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [[MFMailComposeViewController alloc] init];
    vc.mailComposeDelegate = self;
    [vc setSubject:@"MAPS.ME"];
    [vc setToRecipients:@[email]];
    [vc setMessageBody:text isHTML:NO];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
    NSString * text = [NSString stringWithFormat:NSLocalizedString(@"email_error_body", nil), email];
    [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"email_error_title", nil) message:text delegate:nil cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

@end
