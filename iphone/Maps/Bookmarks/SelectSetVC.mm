#import "SelectSetVC.h"
#import "BalloonView.h"
#import "Framework.h"
#import "AddSetVC.h"

@implementation SelectSetVC

- (id) initWithBalloonView:(BalloonView *)view andEditMode:(BOOL)enabled
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    m_editModeEnabled = enabled;
    
    self.title = NSLocalizedString(@"bookmark_sets", @"Bookmark Sets dialog title");
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  if (m_editModeEnabled)
  {
    // Do not show Edit button if we have only one bookmarks set
    if (GetFramework().GetBmCategoriesCount() > 1)
      self.navigationItem.rightBarButtonItem = self.editButtonItem;
  }

  [super viewWillAppear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  // Hide Add New Set button if Edit is not enabled
  return m_editModeEnabled ? 2 : 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  NSInteger count = GetFramework().GetBmCategoriesCount();
  // If no bookmarks are added, display default set
  if (count == 0)
    count = 1;

  if (section == 0)
    return m_editModeEnabled ? 1 : count;
  return count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  static NSString * kSetCellId = @"AddSetCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:kSetCellId];
  if (cell == nil)
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kSetCellId] autorelease];
  // Customize cell
  if (indexPath.section == 0 && m_editModeEnabled)
  {
    cell.textLabel.text = NSLocalizedString(@"add_new_set", @"Bookmark Sets dialog - Add New Set button");
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }
  else
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(indexPath.row);
    if (cat)
      cell.textLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];
    else
      cell.textLabel.text = m_balloon.setName; // Use "not existing" default set

    if ([m_balloon.setName isEqualToString:cell.textLabel.text])
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [cell setSelected:NO animated:YES];
  if (indexPath.section == 0 && m_editModeEnabled)
  {
    AddSetVC * asVC = [[AddSetVC alloc] initWithBalloonView:m_balloon];
    [self.navigationController pushViewController:asVC animated:YES];
    [asVC release];
  }
  else
  {
    if (![m_balloon.setName isEqualToString:cell.textLabel.text])
    {
      m_balloon.setName = cell.textLabel.text;
      // Change visible bookmarks category
      GetFramework().SetVisibleBmCategory([m_balloon.setName UTF8String]);
      // Update Bookmarks VC if needed
      if (!m_editModeEnabled)
      {
        NSArray * vcs = self.navigationController.viewControllers;
        UITableViewController * bmVC = (UITableViewController *)[vcs objectAtIndex:[vcs count] - 2];
        [bmVC.tableView reloadData];
      }
    }
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Return NO if you do not want the specified item to be editable.
  if (indexPath.section == 0)
    return NO;
  return YES;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 1)
  {
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
      // Move checkmark to another category if we're deleting the checked one
      Framework & f = GetFramework();
      BookmarkCategory * cat = f.GetBmCategory(indexPath.row);
      bool moveCheckMark = false;
      if (cat && cat->GetName() == [m_balloon.setName UTF8String])
        moveCheckMark = true;

      if (f.DeleteBmCategory(indexPath.row))
        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
      if (f.GetBmCategoriesCount() == 1)
      {
        // Disable edit mode to leave at least one bookmarks category
        [self setEditing:NO animated:YES];
        self.navigationItem.rightBarButtonItem = nil;
      }
      if (moveCheckMark)
      {
        UITableViewCell * cell = [tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:1]];
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
        m_balloon.setName = cell.textLabel.text;
      }
    }
  }
}

@end
