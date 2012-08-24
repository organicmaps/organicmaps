#import "PlacePageVC.h"
#import "BalloonView.h"
#import "SelectSetVC.h"
#import "SelectColorVC.h"

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
  // Update the table - we can display it after changing set or color
  [self.tableView reloadData];
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
  switch (section)
  {
  case 0: return 3;
  case 1: return 2;
  default: return 0;
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (section != 0)
    return nil;
  // Address and type text
  UILabel * label = [[[UILabel alloc] initWithFrame:CGRectMake(0, 0, tableView.frame.size.width, 40)] autorelease];
  label.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  label.numberOfLines = 0;
  label.lineBreakMode = UILineBreakModeWordWrap;
  label.backgroundColor = [UIColor clearColor];
  label.textColor = [UIColor darkGrayColor];
  label.textAlignment = UITextAlignmentCenter;
  label.text = [NSString stringWithFormat:@"%@\n%@", m_balloon.type, m_balloon.description];
  return label;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if (section != 0)
    return 0;
  return 60;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell;
  if (indexPath.section == 0)
  {
    NSString * cellId = @"DefaultCell";
    switch (indexPath.row)
    {
      case 0: cellId = @"NameCell"; break;
      case 1: cellId = @"SetCell"; break;
      case 2: cellId = @"ColorCell"; break;
    }

    cell = [tableView dequeueReusableCellWithIdentifier:cellId];
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
          f.placeholder = NSLocalizedString(@"name", @"Add bookmark dialog - bookmark name");
          f.textColor = cell.detailTextLabel.textColor;
          cell.accessoryView = f;
          cell.textLabel.text = NSLocalizedString(@"name", @"Add bookmark dialog - bookmark name");
          cell.selectionStyle = UITableViewCellSelectionStyleNone;
        }
        break;

        case 1:
          cell.textLabel.text = NSLocalizedString(@"Set", @"Add bookmark dialog - bookmark set");
        break;

        case 2:
          cell.textLabel.text = NSLocalizedString(@"color", @"Add bookmark dialog - bookmark color");
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
        cell.detailTextLabel.text = m_balloon.setName;
      break;

      case 2:
        // Create a copy of view here because it can't be subview in map view and in cell simultaneously
        cell.accessoryView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:m_balloon.color]] autorelease];
      break;
    }
  }
  else
  {
    // 2nd section with add/remove pin buttons
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"cell"] autorelease];
    cell.textLabel.textAlignment = UITextAlignmentCenter;
    if (indexPath.row == 0)
      cell.textLabel.text = NSLocalizedString(@"add_to_bookmarks", @"Place Page - Add To Bookmarks button");
    else
      cell.textLabel.text = NSLocalizedString(@"remove_pin", @"Place Page - Remove Pin button");
  }
  return cell;
}

- (void)onAddClicked
{
  GetFramework().AddBookmark([m_balloon.setName UTF8String],
                             Bookmark(m2::PointD(m_balloon.globalPosition.x, m_balloon.globalPosition.y),
                                      [m_balloon.title UTF8String], [m_balloon.color UTF8String]));
  [m_balloon hide];
//  // Don't forget to hide navbar
//  [self.navigationController setNavigationBarHidden:YES animated:YES];
//  [self.navigationController popToRootViewControllerAnimated:YES];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [[tableView cellForRowAtIndexPath:indexPath] setSelected:NO animated:YES];

  if (indexPath.section == 0)
  {
    switch (indexPath.row)
    {
      case 1:
      {
        m_hideNavBar = NO;
        SelectSetVC * vc = [[SelectSetVC alloc] initWithBalloonView:m_balloon andEditMode:YES];
        [self.navigationController pushViewController:vc animated:YES];
        [vc release];
      }
      break;

      case 2:
      {
        m_hideNavBar = NO;
        SelectColorVC * vc = [[SelectColorVC alloc] initWithBalloonView:m_balloon];
        [self.navigationController pushViewController:vc animated:YES];
        [vc release];
      }
      break;
    }
  }
  else
  {
    if (indexPath.row == 0)
    {
      // Add to bookmarks
      [self onAddClicked];
    }
    else
    {
      // Remove pin
      [m_balloon hide];
    }
    // Close place page
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  if (m_balloon.title != textField.text)
  {
    if (textField.text.length == 0)
    {
      m_balloon.title = NSLocalizedString(@"dropped_pin", @"Unknown Dropped Pin title, when name can't be determined");
      textField.text = m_balloon.title;
    }
    else
      m_balloon.title = textField.text;
    self.navigationController.title = m_balloon.title;
    [m_balloon updateTitle:m_balloon.title];
  }
  return NO;
}
@end
