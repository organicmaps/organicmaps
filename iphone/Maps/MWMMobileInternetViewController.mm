#import "MWMMobileInternetViewController.h"
#import "MWMNetworkingPolicy.h"
#import "SelectableCell.h"
#import "Statistics.h"

using namespace networking_policy;

@interface MWMMobileInternetViewController ()

@property(weak, nonatomic) IBOutlet SelectableCell * always;
@property(weak, nonatomic) IBOutlet SelectableCell * ask;
@property(weak, nonatomic) IBOutlet SelectableCell * never;
@property(weak, nonatomic) SelectableCell * selected;

@end

@implementation MWMMobileInternetViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_mobile_internet");

  SelectableCell * selected;
  switch (GetNetworkingPolicyState())
  {
  case NetworkingPolicyState::Always: selected = self.always; break;
  case NetworkingPolicyState::Session: selected = self.ask; break;
  case NetworkingPolicyState::Never: selected = self.never; break;
  }
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selected = selected;
}

- (void)setSelected:(SelectableCell *)selected
{
  if ([_selected isEqual:selected])
    return;

  _selected = selected;
  NSString * statValue = nil;
  if ([selected isEqual:self.always])
  {
    statValue = kStatAlways;
    SetNetworkingPolicyState(NetworkingPolicyState::Always);
  }
  else if ([selected isEqual:self.ask])
  {
    statValue = kStatAsk;
    SetNetworkingPolicyState(NetworkingPolicyState::Session);
  }
  else if ([selected isEqual:self.never])
  {
    statValue = kStatNever;
    SetNetworkingPolicyState(NetworkingPolicyState::Never);
  }

  [Statistics logEvent:kStatMobileInternet withParameters:@{kStatValue : statValue}];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  SelectableCell * selected = self.selected;
  selected.accessoryType = UITableViewCellAccessoryNone;
  selected = [tableView cellForRowAtIndexPath:indexPath];
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  selected.selected = NO;
  self.selected = selected;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"pref_mobile_internet_hint");
}

@end
