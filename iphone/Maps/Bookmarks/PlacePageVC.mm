#import "PlacePageVC.h"
#import "AddBookmarkVC.h"
#import "BalloonView.h"

@implementation PlacePageVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;

    self.title = m_balloon.title;
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
  return 3;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell;
  if (indexPath.section == 0)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"AddressCell"] autorelease];
    cell.textLabel.text = NSLocalizedString(@"Address", @"Place Page - Address cell");
    cell.detailTextLabel.numberOfLines = 0;
    cell.detailTextLabel.textAlignment = UITextAlignmentLeft;
    cell.detailTextLabel.text = m_balloon.description;
  }
  else
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"cell"] autorelease];
    cell.textLabel.textAlignment = UITextAlignmentCenter;
    if (indexPath.section == 1)
      cell.textLabel.text = NSLocalizedString(@"Add To Bookmarks", @"Place Page - Add To Bookmarks button");
    else
      cell.textLabel.text = NSLocalizedString(@"Remove Pin", @"Place Page - Remove Pin button");
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];

  switch (indexPath.section)
  {
    case 1:
    {
      AddBookmarkVC * addVC = [[AddBookmarkVC alloc] initWithBalloonView:m_balloon];
      m_hideNavBar = NO;
      [self.navigationController pushViewController:addVC animated:YES];
      [addVC release];
    }
    break;

    case 2:
    {
      [m_balloon hide];
      [self.navigationController popViewControllerAnimated:YES];
    }
    break;
  }
}

@end
