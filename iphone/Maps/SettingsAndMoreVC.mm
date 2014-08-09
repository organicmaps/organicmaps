
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

  self.items = @[@{@"Id" : @"About", @"Title" : NSLocalizedString(@"about_menu_title", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Community", @"Title" : NSLocalizedString(@"maps_me_community", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"RateApp", @"Title" : NSLocalizedString(@"rate_the_app", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Settings", @"Title" : NSLocalizedString(@"settings", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"ReportBug", @"Title" : NSLocalizedString(@"report_a_bug", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Help", @"Title" : NSLocalizedString(@"help", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Copyright", @"Title" : NSLocalizedString(@"copyright", nil), @"Icon" : @"MWMProIcon"}];
}

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self.items count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSDictionary * item = self.items[indexPath.row];

  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  if (!cell) // iOS 5
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[UITableViewCell className]];

  cell.textLabel.text = item[@"Title"];
  cell.imageView.image = [UIImage imageNamed:item[@"Icon"]];

  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * itemId = self.items[indexPath.row][@"Id"];
  if ([itemId isEqualToString:@"About"])
  {
    [self about];
  }
  else if ([itemId isEqualToString:@"Community"])
  {
    CommunityVC * vc = [[CommunityVC alloc] init];
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
    [self reportBug];
  }
  else if ([itemId isEqualToString:@"Help"])
  {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
  }
  else if ([itemId isEqualToString:@"Copyright"])
  {
    [self copyright];
  }
}

- (void)about
{
  RichTextVC * vc = [[RichTextVC alloc] initWithText:NSLocalizedString(@"about_text", nil)];
  vc.title = NSLocalizedString(@"about_menu_title", nil);
  [self.navigationController pushViewController:vc animated:YES];
}

- (void)copyright
{
  ReaderPtr<Reader> r = GetPlatform().GetReader("about.html");
  string s;
  r.ReadAsString(s);
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
    [[UIApplication sharedApplication] rateProVersionFrom:@"ios_popup"];
  else
    [[UIApplication sharedApplication] rateLiteVersionFrom:@"ios_popup"];
}

- (void)reportBug
{
  struct utsname systemInfo;
  uname(&systemInfo);
  NSString * machine = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
  NSString * text = [NSString stringWithFormat:@"\n\n\n\n- %@ (%@)\n- maps.me %@", self.deviceNames[machine], [UIDevice currentDevice].systemVersion, [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey]];
  NSString * email = @"bugs@maps.me";
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [MFMailComposeViewController new];
    vc.mailComposeDelegate = self;
    [vc setSubject:@"maps.me"];
    [vc setToRecipients:@[email]];
    [vc setMessageBody:text isHTML:NO];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
    #warning translation
    NSString * text = [NSString stringWithFormat:@"!!!Почтовый клиент не настроен. Попробуйте написать нам другим способом на %@", email];
    [[[UIAlertView alloc] initWithTitle:@"!!!Не настроена почта" message:text delegate:nil cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

@end
