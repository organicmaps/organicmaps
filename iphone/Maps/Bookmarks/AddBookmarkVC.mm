#import "AddBookmarkVC.h"
#import "BalloonView.h"
#import "Framework.h"
#import "SelectSetVC.h"


@implementation AddBookmarkVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(onAddClicked)];
    self.title = NSLocalizedString(@"Add Bookmark", @"Add bookmark dialog title");
  }
  return self;
}

- (void)onAddClicked
{
  GetFramework().AddBookmark([m_balloon.setName UTF8String],
                      Bookmark(m2::PointD(m_balloon.globalPosition.x, m_balloon.globalPosition.y),
                      [m_balloon.title UTF8String]));
  [m_balloon hide];
  // Don't forget to hide navbar
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  [self.navigationController popToRootViewControllerAnimated:YES];
}

- (void)viewWillAppear:(BOOL)animated
{
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  // Update the table - we can display it after changing set or color
  [self.tableView reloadData];
  [super viewWillAppear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 3;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * cellId = @"DefaultCell";
  switch (indexPath.row)
  {
    case 0: cellId = @"NameCell"; break;
    case 1: cellId = @"SetCell"; break;
    case 2: cellId = @"ColorCell"; break;
  }

  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:cellId];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:cellId] autorelease];
    switch (indexPath.row)
    {
      case 0:
      {      
        UITextField * f = [[[UITextField alloc] initWithFrame:CGRectMake(0, 0, 200, 21)] autorelease];
        f.textAlignment = UITextAlignmentRight;
        f.returnKeyType = UIReturnKeyDone;
        f.clearButtonMode = UITextFieldViewModeWhileEditing;
        f.autocorrectionType = UITextAutocorrectionTypeNo;
        f.delegate = self;
        f.placeholder = NSLocalizedString(@"Name", @"Add bookmark dialog - bookmark name");
        f.textColor = cell.detailTextLabel.textColor;
        cell.accessoryView = f;
        cell.textLabel.text = NSLocalizedString(@"Name", @"Add bookmark dialog - bookmark name");
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
      }
      break;

      case 1:
        cell.textLabel.text = NSLocalizedString(@"Set", @"Add bookmark dialog - bookmark set");
      break;

      case 2:
        cell.textLabel.text = NSLocalizedString(@"Color", @"Add bookmark dialog - bookmark color");
      break;
    }
  }
  // Update variable cell values
  switch (indexPath.row)
  {
    case 0:
      ((UITextField *)(cell.accessoryView)).text = m_balloon.title;
    break;

    case 1:
      cell.detailTextLabel.text = [m_balloon setName];
    break;

    case 2:
      cell.accessoryView = [m_balloon pinImage];
    break;
  }
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];
  
  if (indexPath.row == 1)
  {
    SelectSetVC * vc = [[SelectSetVC alloc] initWithBalloonView:m_balloon andEditMode:YES];
    [self.navigationController pushViewController:vc animated:YES];
    [vc release];
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  return NO;
}

@end
