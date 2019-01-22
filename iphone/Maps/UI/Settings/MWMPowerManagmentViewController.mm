#import "MWMPowerManagmentViewController.h"

#import "SwiftBridge.h"

#include "Framework.h"

using namespace power_management;

@interface MWMPowerManagmentViewController ()
@property (weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * never;
@property (weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * manualMax;
@property (weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * automatic;
@property (weak, nonatomic) SettingsTableViewSelectableCell * selected;

@end

@implementation MWMPowerManagmentViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"power_management_title");

  SettingsTableViewSelectableCell * selected;
  switch (GetFramework().GetPowerManager().GetScheme())
  {
  case Scheme::None: break;
  case Scheme::Normal: selected = self.never; break;
  case Scheme::EconomyMedium: break;
  case Scheme::EconomyMaximum: selected = self.manualMax; break;
  case Scheme::Auto: selected = self.automatic; break;
  }
  selected.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selected = selected;
}

- (void)setSelected:(SettingsTableViewSelectableCell *)selected
{
  if ([_selected isEqual:selected])
    return;

  _selected = selected;
  if ([selected isEqual:self.never])
    GetFramework().GetPowerManager().SetScheme(Scheme::Normal);
  else if ([selected isEqual:self.manualMax])
    GetFramework().GetPowerManager().SetScheme(Scheme::EconomyMaximum);
  else if ([selected isEqual:self.automatic])
    GetFramework().GetPowerManager().SetScheme(Scheme::Auto);
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
  return L(@"power_management_description");
}

@end
