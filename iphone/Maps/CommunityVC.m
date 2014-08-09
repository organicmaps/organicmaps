
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

  self.items = @[@{@"Id" : @"Facebook", @"Title" : NSLocalizedString(@"like_on_facebook", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Twitter", @"Title" : NSLocalizedString(@"follow_on_twitter", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Contact", @"Title" : NSLocalizedString(@"contact_us", nil), @"Icon" : @"MWMProIcon"},
                 @{@"Id" : @"Subscribe", @"Title" : NSLocalizedString(@"subscribe_to_news", nil), @"Icon" : @"MWMProIcon"}];
}

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
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [MFMailComposeViewController new];
    vc.mailComposeDelegate = self;
    [vc setSubject:@"maps.me"];
    [vc setToRecipients:@[@"info@maps.me"]];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
#warning translation
    NSString * text = [NSString stringWithFormat:@"!!!Почтовый клиент не настроен. Попробуйте написать нам другим способом"];
    [[[UIAlertView alloc] initWithTitle:@"!!!Не настроена почта" message:text delegate:nil cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil] show];
  }
}

- (void)subscribe
{
  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * vc = [MFMailComposeViewController new];
    vc.mailComposeDelegate = self;
    [vc setSubject:NSLocalizedString(@"subscribe_me_subject", nil)];
    [vc setToRecipients:@[@"subscribe@maps.me"]];
    [vc setMessageBody:NSLocalizedString(@"subscribe_me_body", nil) isHTML:NO];
    [self presentViewController:vc animated:YES completion:nil];
  }
  else
  {
#warning translation
    NSString * text = [NSString stringWithFormat:@"!!!Почтовый клиент не настроен. Попробуйте написать нам другим способом"];
    [[[UIAlertView alloc] initWithTitle:@"!!!Не настроена почта" message:text delegate:nil cancelButtonTitle:NSLocalizedString(@"ok", nil) otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

@end
