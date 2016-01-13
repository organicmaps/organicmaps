#import "CommunityVC.h"
#import <MessageUI/MFMailComposeViewController.h>
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIImageView+Coloring.h"
#import "UIViewController+Navigation.h"

#import "../../3party/Alohalytics/src/alohalytics_objc.h"

extern NSString * const kAlohalyticsTapEventKey;

@interface CommunityVC () <MFMailComposeViewControllerDelegate>

@property (nonatomic) NSArray * items;

@end

@implementation CommunityVC

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"maps_me_community");

  self.items = @[@{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Facebook", @"Title" : L(@"like_on_facebook"), @"Icon" : @"IconFacebook"},
                                @{@"Id" : @"Twitter", @"Title" : L(@"follow_on_twitter"), @"Icon" : @"IconTwitter"},
                                @{@"Id" : @"Subscribe", @"Title" : L(@"subscribe_to_news"), @"Icon" : @"IconSubscribe"}]},
                 @{@"Title" : @"",
                   @"Items" : @[@{@"Id" : @"Contact", @"Title" : L(@"contact_us"), @"Icon" : @"IconReportABug"}]}];
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
  cell.imageView.mwm_coloring = MWMImageColoringBlack;
  cell.backgroundColor = [UIColor white];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * itemId = self.items[indexPath.section][@"Items"][indexPath.row][@"Id"];
  [[Statistics instance] logEvent:kStatEventName(kStatSocial, kStatToggleCompassCalibration)
                   withParameters:@{kStatValue : itemId}];
  if ([itemId isEqualToString:@"Facebook"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"likeOnFb"];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://facebook.com/MapsWithMe"]];
  }
  else if ([itemId isEqualToString:@"Twitter"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"followOnTwitter"];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:@"https://twitter.com/MAPS_ME"]];
  }
  else if ([itemId isEqualToString:@"Contact"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"contactUs"];
    [self contact];
  }
  else if ([itemId isEqualToString:@"Subscribe"])
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"subscribeToNews"];
    [self subscribe];
  }
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (void)contact
{
  [self sendEmailWithText:nil subject:@"MAPS.ME" toRecipient:@"ios@maps.me"];
}

- (void)subscribe
{
  [self sendEmailWithText:L(@"subscribe_me_body") subject:L(@"subscribe_me_subject") toRecipient:@"subscribe@maps.me"];
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
    NSString * text = [NSString stringWithFormat:L(@"email_error_body"), email];
    [[[UIAlertView alloc] initWithTitle:L(@"email_error_title") message:text delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
  }
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self dismissViewControllerAnimated:YES completion:nil];
}

@end
