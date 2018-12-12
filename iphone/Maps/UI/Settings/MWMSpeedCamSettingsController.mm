#import "MWMSpeedCamSettingsController.h"
#import "Statistics.h"
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
  auto mode = SpeedCameraManagerMode::MaxValue;
  if (selectedCell == self.autoCell)
    mode = SpeedCameraManagerMode::Auto;
  else if (selectedCell == self.alwaysCell)
    mode = SpeedCameraManagerMode::Always;
  else
    mode = SpeedCameraManagerMode::Never;

  CHECK_NOT_EQUAL(mode, SpeedCameraManagerMode::MaxValue, ());
  scm.SetMode(mode);
  [Statistics logEvent:kStatSettingsSpeedCameras
        withParameters:@{kStatValue : @(DebugPrint(mode).c_str())}];
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  _selectedCell = selectedCell;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

#pragma mark - UITableViewDataSource

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  switch (section)
  {
  case 0: return L(@"speedcams_notice_message");
  default: return nil;
  }
}

@end
