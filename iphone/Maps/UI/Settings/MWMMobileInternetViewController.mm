#import "MWMMobileInternetViewController.h"
#import "MWMNetworkPolicy.h"
#import "Statistics.h"
#import "SwiftBridge.h"

using namespace network_policy;
using np = platform::NetworkPolicy;

@interface MWMMobileInternetViewController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * always;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * ask;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * never;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selected;

@end

@implementation MWMMobileInternetViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"mobile_data");

  SettingsTableViewSelectableCell * selected;
  switch (GetStage())
  {
  case np::Stage::Always: selected = self.always; break;
  case np::Stage::Session: selected = self.ask; break;
  case np::Stage::Never: selected = self.never; break;
  }
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selected = selected;
}

- (void)setSelected:(SettingsTableViewSelectableCell *)selected
{
  if ([_selected isEqual:selected])
    return;

  _selected = selected;
  NSString * statValue = @"";
  if ([selected isEqual:self.always])
  {
    statValue = kStatAlways;
    SetStage(np::Stage::Always);
  }
  else if ([selected isEqual:self.ask])
  {
    statValue = kStatAsk;
    SetStage(np::Stage::Session);
  }
  else if ([selected isEqual:self.never])
  {
    statValue = kStatNever;
    SetStage(np::Stage::Never);
  }

  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : statValue}];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  SettingsTableViewSelectableCell * selected = self.selected;
  selected.accessoryType = UITableViewCellAccessoryNone;
  selected = [tableView cellForRowAtIndexPath:indexPath];
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  selected.selected = NO;
  self.selected = selected;
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"mobile_data_description");
}

@end
