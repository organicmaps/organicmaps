#import "MWMNightModeController.h"
#import "MWMSettings.h"
#import "SwiftBridge.h"

@interface MWMNightModeController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * autoSwitch;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * on;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * off;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

@end

@implementation MWMNightModeController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_map_style_title");
  SettingsTableViewSelectableCell * selectedCell = nil;
  switch ([MWMSettings theme])
  {
  case MWMThemeVehicleDay: NSAssert(false, @"Invalid case");
  case MWMThemeDay: selectedCell = self.off; break;
  case MWMThemeVehicleNight: NSAssert(false, @"Invalid case");
  case MWMThemeNight: selectedCell = self.on; break;
  case MWMThemeAuto: selectedCell = self.autoSwitch; break;
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell = selectedCell;
}

- (void)setSelectedCell:(SettingsTableViewSelectableCell *)cell
{
  if ([_selectedCell isEqual:cell])
    return;

  _selectedCell = cell;
  if ([cell isEqual:self.on])
    [MWMSettings setTheme:MWMThemeNight];
  else if ([cell isEqual:self.off])
    [MWMSettings setTheme:MWMThemeDay];
  else if ([cell isEqual:self.autoSwitch])
    [MWMSettings setTheme:MWMThemeAuto];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  SettingsTableViewSelectableCell * selectedCell = self.selectedCell;
  selectedCell.accessoryType = UITableViewCellAccessoryNone;
  selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  selectedCell.selected = NO;
  self.selectedCell = selectedCell;
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

@end
