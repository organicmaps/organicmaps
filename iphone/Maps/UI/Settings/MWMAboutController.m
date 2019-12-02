#import "MWMAboutController.h"

#import <CoreApi/MWMFrameworkHelper.h>

#import "Statistics.h"
#import "SwiftBridge.h"

@interface MWMAboutController () <SettingsTableViewSwitchCellDelegate>

@property(weak, nonatomic) IBOutlet UILabel * versionLabel;
@property(weak, nonatomic) IBOutlet UILabel * dataVersionLabel;

@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * websiteCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * facebookCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * twitterCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * osmCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * rateCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * adsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSwitchCell * crashlyticsCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * privacyPolicyCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * termsOfUseCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewLinkCell * copyrightCell;

@property(nonatomic) IBOutlet UIView * headerView;

@end

@implementation MWMAboutController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"about_menu_title");

  [NSBundle.mainBundle loadNibNamed:@"MWMAboutControllerHeader" owner:self options:nil];
  self.tableView.tableHeaderView = self.headerView;

  AppInfo * appInfo = [AppInfo sharedInfo];
  NSString * version = appInfo.bundleVersion;
  if (appInfo.buildNumber)
    version = [NSString stringWithFormat:@"%@.%@", version, appInfo.buildNumber];
  self.versionLabel.text = [NSString stringWithFormat:L(@"version"), version];

  self.dataVersionLabel.text = [NSString stringWithFormat:L(@"data_version"), [MWMFrameworkHelper dataVersion]];

  [self.crashlyticsCell configWithDelegate:self title:L(@"opt_out_fabric") isOn:![MWMSettings crashReportingDisabled]];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  SettingsTableViewLinkCell *cell = [tableView cellForRowAtIndexPath:indexPath];
  if (cell == self.websiteCell)
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"website"];
    [self openUrl:[NSURL URLWithString:@"https://maps.me"]];
  }
  else if (cell == self.facebookCell)
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"likeOnFb"];
    [self openUrl:[NSURL URLWithString:@"https://facebook.com/MapsWithMe"]];
  }
  else if (cell == self.twitterCell)
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"followOnTwitter"];
    [self openUrl:[NSURL URLWithString:@"https://twitter.com/MAPS_ME"]];
  }
  else if (cell == self.osmCell)
  {
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"osm"];
    [self openUrl:[NSURL URLWithString:@"https://www.openstreetmap.org"]];
  }
  else if (cell == self.rateCell)
  {
    [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatRate}];
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"rate"];
    [UIApplication.sharedApplication rateApp];
  }
  else if (cell == self.privacyPolicyCell)
  {
    [self openUrl:[NSURL URLWithString:@"https://legal.my.com/us/maps/privacy/"]];
  }
  else if (cell == self.termsOfUseCell)
  {
    [self openUrl:[NSURL URLWithString:@"https://legal.my.com/us/maps/tou/"]];
  }
  else if (cell == self.copyrightCell)
  {
    [Statistics logEvent:kStatSettingsOpenSection withParameters:@{kStatName : kStatCopyright}];
    [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"copyright"];
    NSError * error;
    NSString * filePath = [[NSBundle mainBundle] pathForResource:@"copyright" ofType:@"html"];
    NSString * s = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:&error];
    NSString * text = [NSString stringWithFormat:@"%@\n%@", self.versionLabel.text, s];
    WebViewController * aboutViewController =
        [[WebViewController alloc] initWithHtml:text baseUrl:nil title:L(@"copyright")];
    aboutViewController.openInSafari = YES;
    [self.navigationController pushViewController:aboutViewController animated:YES];
  }
  else if (cell == self.adsCell)
  {
    [Statistics logEvent:@"Settings_Tracking_details"
          withParameters:@{kStatType: @"personal_ads"}];
  }
}

#pragma mark - Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return section == 2 ? L(@"subtittle_opt_out") : nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return section == 2 ? L(@"opt_out_fabric_description") : nil;
}

#pragma mark - SettingsTableViewSwitchCellDelegate

- (void)switchCell:(SettingsTableViewSwitchCell *)cell didChangeValue:(BOOL)value
{
  if (cell == self.crashlyticsCell)
  {
    [Statistics logEvent:@"Settings_Tracking_toggle"
          withParameters:@{kStatType: @"crash_reports",
                           kStatValue: value ? @"on" : @"off"}];
    [MWMSettings setCrashReportingDisabled:!value];
  }
}

@end
