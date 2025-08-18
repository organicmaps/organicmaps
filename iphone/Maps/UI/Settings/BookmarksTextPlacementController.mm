#import "BookmarksTextPlacementController.h"
#import "MWMSettings.h"
#import "SwiftBridge.h"

@interface BookmarksTextPlacementController ()

@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * hide;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * showOnTheRight;
@property(weak, nonatomic) IBOutlet SettingsTableViewSelectableCell * showAtTheBottom;
@property(weak, nonatomic) SettingsTableViewSelectableCell * selectedCell;

@end

@implementation BookmarksTextPlacementController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"bookmarks_text_placement_title");

  SettingsTableViewSelectableCell * selectedCell;
  switch ([MWMSettings bookmarksTextPlacement])
  {
  case MWMPlacementNone: selectedCell = self.hide; break;
  case MWMPlacementRight: selectedCell = self.showOnTheRight; break;
  case MWMPlacementBottom: selectedCell = self.showAtTheBottom; break;
  }
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedCell = selectedCell;
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
  if (cell == self.hide)
    [MWMSettings setBookmarksTextPlacement:MWMPlacementNone];
  else if (cell == self.showOnTheRight)
    [MWMSettings setBookmarksTextPlacement:MWMPlacementRight];
  else if (cell == self.showAtTheBottom)
    [MWMSettings setBookmarksTextPlacement:MWMPlacementBottom];
  else
    NSAssert(false, @"Did you forget to add new MWMPlacement setting?");
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedCell = [tableView cellForRowAtIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  return L(@"bookmarks_text_placement_description");
}

@end
