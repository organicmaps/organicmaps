#import "MWMPowerManagmentViewController.h"

#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

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
  self.title = L(@"power_managment_title");

  SettingsTableViewSelectableCell * selectedCell;
  switch (GetFramework().GetPowerManager().GetScheme())
  {
  case power_management::Scheme::None: break;
  case power_management::Scheme::Normal: selectedCell = self.never; break;
  case power_management::Scheme::EconomyMedium: break;
  case power_management::Scheme::EconomyMaximum: selectedCell = self.manualMax; break;
  case power_management::Scheme::Auto: selectedCell = self.automatic; break;
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
  power_management::Scheme scheme = power_management::Scheme::Auto;
  if ([selectedCell isEqual:self.never])
    scheme = power_management::Scheme::Normal;
  else if ([selectedCell isEqual:self.manualMax])
    scheme = power_management::Scheme::EconomyMaximum;

  GetFramework().GetPowerManager().SetScheme(scheme);
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"power_managment_description");
}

@end
