#import "SelectSetVC.h"
#import "BalloonView.h"
#import "Framework.h"
#import "AddSetVC.h"

@implementation SelectSetVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    
    self.title = NSLocalizedString(@"bookmark_sets", @"Bookmark Sets dialog title");
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  // Do not show Edit button if we have only one bookmarks set
  if (GetFramework().GetBmCategoriesCount() > 1)
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

  [super viewWillAppear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  // "Add new set" button
  if (section == 0)
    return 1;

  return GetFramework().GetBmCategoriesCount();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  static NSString * kSetCellId = @"AddSetCell";
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:kSetCellId];
  if (cell == nil)
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:kSetCellId] autorelease];
  // Customize cell
  if (indexPath.section == 0)
  {
    cell.textLabel.text = NSLocalizedString(@"add_new_set", @"Bookmark Sets dialog - Add New Set button");
    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
  }
  else
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(indexPath.row);
    if (cat)
      cell.textLabel.text = [NSString stringWithUTF8String:cat->GetName().c_str()];

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
  if (indexPath.section == 0)
  {
    AddSetVC * asVC = [[AddSetVC alloc] initWithBalloonView:m_balloon andRootNavigationController:self.navigationController];
    // Use temporary navigation controller to display nav bar in modal view controller
    UINavigationController * navC = [[UINavigationController alloc] initWithRootViewController:asVC];
    [self.navigationController presentModalViewController:navC animated:YES];
    [navC release];
    [asVC release];
  }
  else
  {
    if (![m_balloon.setName isEqualToString:cell.textLabel.text])
      m_balloon.setName = cell.textLabel.text;
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Return NO if you do not want the specified item to be editable.
  if (indexPath.section == 0)
    return NO;
  // Disable deleting of the last remaining set (can be activated by swipe right on a set item)
  if (GetFramework().GetBmCategoriesCount() > 1)
    return YES;
  return NO;
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
