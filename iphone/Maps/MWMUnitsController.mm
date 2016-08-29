#import "MWMUnitsController.h"
#import "MWMSettings.h"
#import "SelectableCell.h"
#import "Statistics.h"

@interface MWMUnitsController ()

@property(weak, nonatomic) IBOutlet SelectableCell * kilometers;
@property(weak, nonatomic) IBOutlet SelectableCell * miles;
@property(weak, nonatomic) SelectableCell * selectedCell;

@end

@implementation MWMUnitsController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"measurement_units");

  switch ([MWMSettings measurementUnits])
  {
  case measurement_utils::Units::Metric: self.selectedCell = self.kilometers; break;
  case measurement_utils::Units::Imperial: self.selectedCell = self.miles; break;
  }
}

- (void)setSelectedCell:(SelectableCell *)cell
{
  if ([_selectedCell isEqual:cell])
    return;

  _selectedCell.accessoryType = UITableViewCellAccessoryNone;
  _selectedCell = cell;
  _selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  _selectedCell.selected = NO;
  if (cell == self.kilometers)
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
          withParameters:@{kStatValue : kStatKilometers}];
    [MWMSettings setMeasurementUnits:measurement_utils::Units::Metric];
  }
  else
  {
    [Statistics logEvent:kStatEventName(kStatSettings, kStatChangeMeasureUnits)
          withParameters:@{kStatValue : kStatMiles}];
    [MWMSettings setMeasurementUnits:measurement_utils::Units::Imperial];
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
}

@end
