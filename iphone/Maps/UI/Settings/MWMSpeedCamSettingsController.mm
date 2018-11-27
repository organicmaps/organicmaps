#import "MWMSpeedCamSettingsController.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "routing/speed_camera_manager.hpp"

#include "map/routing_manager.hpp"

#include "base/assert.hpp"

@interface MWMSpeedCamSettingsController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * autoCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * alwaysCell;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * neverCell;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

@end

using namespace routing;

@implementation MWMSpeedCamSettingsController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"speedcams_alert_title");
  switch (GetFramework().GetRoutingManager().GetSpeedCamManager().GetMode())
  {
  case SpeedCameraManagerMode::Auto:
    _selectedCell = self.autoCell;
    break;
  case SpeedCameraManagerMode::Always:
    _selectedCell = self.alwaysCell;
    break;
  case SpeedCameraManagerMode::Never:
    _selectedCell = self.neverCell;
    break;
  case SpeedCameraManagerMode::MaxValue:
    CHECK(false, ("Invalid SpeedCameraManagerMode."));
    break;
  }

  auto selectedCell = self.selectedCell;
  CHECK(selectedCell, ());
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
}

- (void)setSelectedCell:(SettingsTableViewSelectableCell *)selectedCell
{
  if (self.selectedCell == selectedCell)
    return;

  _selectedCell.accessoryType = UITableViewCellAccessoryNone;
  auto & scm = GetFramework().GetRoutingManager().GetSpeedCamManager();
  if (selectedCell == self.autoCell)
    scm.SetMode(SpeedCameraManagerMode::Auto);
  else if (selectedCell == self.alwaysCell)
    scm.SetMode(SpeedCameraManagerMode::Always);
  else
    scm.SetMode(SpeedCameraManagerMode::Never);

  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  _selectedCell = selectedCell;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

@end
