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

    if (m_balloon.editedBookmark.first == indexPath.row)
      cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
      cell.accessoryType = UITableViewCellAccessoryNone;
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
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
    [m_balloon deleteBookmark];
    [m_balloon addBookmarkToCategory:indexPath.row];

    [self.navigationController popViewControllerAnimated:YES];
  }
}

@end
