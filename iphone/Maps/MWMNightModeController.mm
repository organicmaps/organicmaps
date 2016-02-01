#import "MapsAppDelegate.h"
#import "MWMNightModeController.h"
#import "SelectableCell.h"
#import "UIColor+MapsMeColor.h"

#include "Framework.h"

@interface MWMNightModeController ()

@property (weak, nonatomic) IBOutlet SelectableCell * autoSwitch;
@property (weak, nonatomic) IBOutlet SelectableCell * on;
@property (weak, nonatomic) IBOutlet SelectableCell * off;
@property (weak, nonatomic) SelectableCell * selectedCell;

@end

@implementation MWMNightModeController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"pref_map_style_title");
  if ([MapsAppDelegate isAutoNightMode])
  {
    self.autoSwitch.accessoryType = UITableViewCellAccessoryCheckmark;
    _selectedCell = self.autoSwitch;
    return;
  }

  switch (GetFramework().GetMapStyle())
  {
  case MapStyleDark:
    self.on.accessoryType = UITableViewCellAccessoryCheckmark;
    _selectedCell = self.on;
    break;
  case MapStyleClear:
  case MapStyleLight:
    self.off.accessoryType = UITableViewCellAccessoryCheckmark;
    _selectedCell = self.off;
    break;
  case MapStyleMerged:
  case MapStyleCount:
    break;
  }
}

- (void)setSelectedCell:(SelectableCell *)cell
{
  if ([_selectedCell isEqual:cell])
    return;

  _selectedCell = cell;
  auto & f = GetFramework();
  auto const style = f.GetMapStyle();
  if ([cell isEqual:self.on])
  {
    [MapsAppDelegate setAutoNightModeOff:YES];
    if (style == MapStyleDark)
      return;
    f.SetMapStyle(MapStyleDark);
    [UIColor setNightMode:YES];
    [self refresh];
  }
  else if ([cell isEqual:self.off])
  {
    [MapsAppDelegate setAutoNightModeOff:YES];
    if (style == MapStyleClear || style == MapStyleLight)
      return;
    f.SetMapStyle(MapStyleClear);
    [UIColor setNightMode:NO];
    [self refresh];
  }
  else if ([cell isEqual:self.autoSwitch])
  {
    [MapsAppDelegate setAutoNightModeOff:NO];
    [MapsAppDelegate changeMapStyleIfNedeed];
    if (style == MapStyleClear || style == MapStyleLight)
      return;
    [UIColor setNightMode:NO];
    f.SetMapStyle(MapStyleClear);
    [self refresh];
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell.accessoryType = UITableViewCellAccessoryNone;
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  self.selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell.selected = NO;
}

@end
