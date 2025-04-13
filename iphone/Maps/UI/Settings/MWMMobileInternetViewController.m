#import "MWMMobileInternetViewController.h"
#import "SwiftBridge.h"

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
  switch ([MWMNetworkPolicy sharedPolicy].permission) {
    case MWMNetworkPolicyPermissionAlways:
      selected = self.always;
      break;
    case MWMNetworkPolicyPermissionNever:
      selected = self.never;
      break;
    case MWMNetworkPolicyPermissionToday:
    case MWMNetworkPolicyPermissionNotToday:
    case MWMNetworkPolicyPermissionAsk:
      selected = self.ask;
      break;
  }
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selected = selected;
}

- (void)setSelected:(SettingsTableViewSelectableCell *)selected
{
  if ([_selected isEqual:selected])
    return;

  _selected = selected;
  if ([selected isEqual:self.always])
    [MWMNetworkPolicy sharedPolicy].permission = MWMNetworkPolicyPermissionAlways;
  else if ([selected isEqual:self.ask])
    [MWMNetworkPolicy sharedPolicy].permission = MWMNetworkPolicyPermissionAsk;
  else if ([selected isEqual:self.never])
    [MWMNetworkPolicy sharedPolicy].permission = MWMNetworkPolicyPermissionNever;
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
