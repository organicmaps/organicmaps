#import "MWMPowerManagmentViewController.h"

#import "SwiftBridge.h"

#include "Framework.h"

using namespace power_management;

@interface MWMPowerManagmentViewController ()
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * never;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * manualMax;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * automatic;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

@end

@implementation MWMPowerManagmentViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"power_management_title");

  SettingsTableViewSelectableCell * selectedCell;
  switch (GetFramework().GetPowerManager().GetScheme())
  {
  case Scheme::None: break;
  case Scheme::Normal: selectedCell = self.never; break;
  case Scheme::EconomyMedium: break;
  case Scheme::EconomyMaximum: selectedCell = self.manualMax; break;
  case Scheme::Auto: selectedCell = self.automatic; break;
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell = selectedCell;
}

- (void)setSelectedCell:(SettingsTableViewSelectableCell *)selectedCell
{
  if (self.selectedCell == selectedCell)
    return;

  _selectedCell.accessoryType = UITableViewCellAccessoryNone;
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;

  _selectedCell = selectedCell;
  if ([selectedCell isEqual:self.never])
    GetFramework().GetPowerManager().SetScheme(Scheme::Normal);
  else if ([selectedCell isEqual:self.manualMax])
    GetFramework().GetPowerManager().SetScheme(Scheme::EconomyMaximum);
  else if ([selectedCell isEqual:self.automatic])
    GetFramework().GetPowerManager().SetScheme(Scheme::Auto);
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"power_management_description");
}

@end
