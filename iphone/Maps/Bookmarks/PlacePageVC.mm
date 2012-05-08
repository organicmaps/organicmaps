#import "PlacePageVC.h"
#import "AddBookmarkVC.h"

@implementation PlacePageVC

- (id)initWithStyle:(UITableViewStyle)style
{
  self = [super initWithStyle:style];
  if (self)
  {
    self.title = NSLocalizedString(@"Place Page", @"Add bookmark dialog title");
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  m_hideNavBar = YES;
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  if (m_hideNavBar)
    [self.navigationController setNavigationBarHidden:YES animated:YES];
  [super viewWillDisappear:animated];
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
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"cell"] autorelease];
  cell.textLabel.textAlignment = UITextAlignmentCenter;
  if (indexPath.section == 0)
    cell.textLabel.text = NSLocalizedString(@"Add To Bookmarks", @"Place Page - Add To Bookmarks button");
  else
    cell.textLabel.text = NSLocalizedString(@"Remove Pin", @"Place Page - Remove Pin button");
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];

  if (indexPath.section == 0)
  {
    AddBookmarkVC * addVC = [[AddBookmarkVC alloc] initWithStyle:UITableViewStyleGrouped];
    m_hideNavBar = NO;
    [self.navigationController pushViewController:addVC animated:YES];
    [addVC release];
  }
  else
  {
    [self.navigationController popViewControllerAnimated:YES];
  }
}

@end
