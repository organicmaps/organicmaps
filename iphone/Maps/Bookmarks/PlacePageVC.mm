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

- (void) dealloc
{
  [m_textField release];
  [super dealloc];
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
  // Handle 3 scenarios:
  // 1. User pressed Remove Pin and goes back to the map - bookmark was deleted on click, do nothing
  // 2. User goes back to the map by pressing Map (Back) button - save possibly edited title, add bookmark
  // 3. User is changing Set or Color - save possibly edited title and update current balloon properties
  if (m_textField)
  {
    if (![m_textField.text isEqualToString:m_balloon.title])
    {
      // Update edited bookmark name
      if (m_textField.text.length == 0)
        m_balloon.title = NSLocalizedString(@"dropped_pin", @"Unknown Dropped Pin title, when name can't be determined");
      else
        m_balloon.title = m_textField.text;
    }

    // We're going back to the map
    if ([self.navigationController.viewControllers indexOfObject:self] == NSNotFound)
    {
      [m_balloon addOrEditBookmark];
      [m_balloon hide];
    }
  }
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
  case 1: return 1;
  default: return 0;
  }
}

//- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
//{
//  if (section != 0)
//    return nil;
//  // Address and type text
//  UILabel * label = [[[UILabel alloc] initWithFrame:CGRectMake(0, 0, tableView.frame.size.width, 40)] autorelease];
//  label.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
//  label.numberOfLines = 0;
//  label.lineBreakMode = UILineBreakModeWordWrap;
//  label.backgroundColor = [UIColor clearColor];
//  label.textColor = [UIColor darkGrayColor];
//  label.textAlignment = UITextAlignmentCenter;
//  label.text = [NSString stringWithFormat:@"%@\n%@", m_balloon.type, m_balloon.description];
//  return label;
//}

//- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
//{
//  if (section != 0)
//    return 0;
//  return 60;
//}

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
          [m_textField release];
          m_textField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, 200, 21)];
          m_textField.textAlignment = UITextAlignmentRight;
          m_textField.returnKeyType = UIReturnKeyDone;
          m_textField.clearButtonMode = UITextFieldViewModeWhileEditing;
          m_textField.autocorrectionType = UITextAutocorrectionTypeNo;
          m_textField.delegate = self;
          m_textField.placeholder = NSLocalizedString(@"name", @"Add bookmark dialog - bookmark name");
          m_textField.textColor = cell.detailTextLabel.textColor;
          cell.accessoryView = m_textField;
          cell.textLabel.text = NSLocalizedString(@"name", @"Add bookmark dialog - bookmark name");
          cell.selectionStyle = UITableViewCellSelectionStyleNone;
        }
        break;

        case 1:
          cell.textLabel.text = NSLocalizedString(@"set", @"Add bookmark dialog - bookmark set");
          cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
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
        m_textField.text = m_balloon.title;
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
    cell.textLabel.text = NSLocalizedString(@"remove_pin", @"Place Page - Remove Pin button");
  }
  return cell;
}

- (void)onRemoveClicked
{
  [m_balloon deleteBookmark];
  [m_balloon hide];
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
        SelectSetVC * vc = [[SelectSetVC alloc] initWithBalloonView:m_balloon];
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
    // Remove pin
    [self onRemoveClicked];
    // Reset text field to indicate that bookmark should not be added on view close
    [m_textField release];
    m_textField = nil;
    // Close place page
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  [textField resignFirstResponder];
  if (![m_balloon.title isEqualToString:textField.text])
  {
    if (textField.text.length == 0)
    {
      m_balloon.title = NSLocalizedString(@"dropped_pin", @"Unknown Dropped Pin title, when name can't be determined");
      textField.text = m_balloon.title;
    }
    else
      m_balloon.title = textField.text;
    self.navigationController.title = m_balloon.title;
  }
  return NO;
}
@end
