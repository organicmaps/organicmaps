
#import "CommunityVC.h"
#import "UIViewController+Navigation.h"
#import "UIKitCategories.h"
#import <MessageUI/MFMailComposeViewController.h>

@interface CommunityVC () <MFMailComposeViewControllerDelegate>

@property (nonatomic) NSArray * items;

@end

@implementation CommunityVC

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.title = NSLocalizedString(@"maps_me_community", nil);

  self.items = @[@{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Facebook", @"Title" : NSLocalizedString(@"like_on_facebook", nil), @"Icon" : @"IconFacebook"},
                                @{@"Id" : @"Twitter", @"Title" : NSLocalizedString(@"follow_on_twitter", nil), @"Icon" : @"IconTwitter"},
                                @{@"Id" : @"Subscribe", @"Title" : NSLocalizedString(@"subscribe_to_news", nil), @"Icon" : @"IconSubscribe"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Contact", @"Title" : NSLocalizedString(@"contact_us", nil), @"Icon" : @"IconReportABug"}]}];
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return 0.001;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return 20;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [self.items count];
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
  if ([itemId isEqualToString:@"Facebook"])
  {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://www.facebook.com/MapsWithMe"]];
  }
  else if ([itemId isEqualToString:@"Twitter"])
  {
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/MAPS_ME"]];
  }
  else if ([itemId isEqualToString:@"Contact"])
  {
    [self contact];
  }
  else if ([itemId isEqualToString:@"Subscribe"])
  {
    [self subscribe];
  }
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (void)contact
{
  [self sendEmailWithText:nil subject:@"MAPS.ME" toRecipient:@"info@maps.me"];
}

- (void)subscribe
{
  [self sendEmailWithText:NSLocalizedString(@"subscribe_me_body", nil) subject:NSLocalizedString(@"subscribe_me_subject", nil) toRecipient:@"subscribe@maps.me"];
}

- (void)sendEmailWithText:(NSString *)text subject:(NSString *)subject toRecipient:(NSString *)email
{
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [[MFMailComposeViewController alloc] init];
    vc.mailComposeDelegate = self;
    [vc setSubject:subject];
    [vc setMessageBody:text isHTML:NO];
    [vc setToRecipients:@[email]];
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
