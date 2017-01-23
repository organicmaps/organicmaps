#import "MWMUnitsController.h"
#import "MWMSettings.h"
#import "Statistics.h"
#import "SwiftBridge.h"

@interface MWMUnitsController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * kilometers;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * miles;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

@end

@implementation MWMUnitsController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"measurement_units");

  switch ([MWMSettings measurementUnits])
  {
  case MWMUnitsMetric: self.selectedCell = self.kilometers; break;
  case MWMUnitsImperial: self.selectedCell = self.miles; break;
  }
}

- (void)setSelectedCell:(SettingsTableViewSelectableCell *)cell
{
  SettingsTableViewSelectableCell * selectedCell = _selectedCell;
  if (selectedCell == cell)
    return;

  selectedCell.accessoryType = UITableViewCellAccessoryNone;
  cell.accessoryType = UITableViewCellAccessoryCheckmark;
  cell.selected = NO;
  _selectedCell = cell;
  if (cell == self.kilometers)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
          withParameters:@{kStatValue : kStatKilometers}];
    [MWMSettings setMeasurementUnits:MWMUnitsMetric];
  }
  else
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
          withParameters:@{kStatValue : kStatMiles}];
    [MWMSettings setMeasurementUnits:MWMUnitsImperial];
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

@end
