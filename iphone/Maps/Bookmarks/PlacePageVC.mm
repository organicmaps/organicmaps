#import "PlacePageVC.h"
#import "BalloonView.h"
#import "SelectSetVC.h"
#import "SelectColorVC.h"

#define TEXTFIELD_TAG 999

@implementation PlacePageVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    self.title = m_balloon.title;
    m_balloon.isCurrentPosition = NO;
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  m_hideNavBar = YES;
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  // Update the table - we can display it after changing set or color
  [self.tableView reloadData];

  // Should be set to YES only if Remove Pin was pressed
  m_removePinOnClose = NO;

  // Automatically show keyboard if bookmark has default name
  if ([m_balloon.title isEqualToString:NSLocalizedString(@"dropped_pin", nil)])
    [[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]].contentView viewWithTag:TEXTFIELD_TAG] becomeFirstResponder];

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
  if (!m_removePinOnClose)
  {
    UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    UITextField * f = (UITextField *)[cell viewWithTag:TEXTFIELD_TAG];
    if (f && f.text.length)
      m_balloon.title = f.text;

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
  if ([self canShare])
    return 3;
  return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  switch (section)
  {
  case 0: return 3;
  case 1: return 1;
  case 2: return 1;
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
  UITableViewCell * cell = nil;
  if (indexPath.section == 0)
  {
    NSString * cellId;
    switch (indexPath.row)
    {
      case 0: cellId = @"NameCellId"; break;
      case 1: cellId = @"SetCellId"; break;
      default: cellId = @"ColorCellId"; break;
    }
    cell = [tableView dequeueReusableCellWithIdentifier:cellId];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:cellId] autorelease];
      switch (indexPath.row)
      {
        case 0:
        {
          cell.textLabel.text = NSLocalizedString(@"name", @"Add bookmark dialog - bookmark name");
          cell.selectionStyle = UITableViewCellSelectionStyleNone;
          // Temporary, to init font and color
          cell.detailTextLabel.text = @"temp string";
          // Called to initialize frames and fonts
          [cell layoutSubviews];
          CGRect const leftR = cell.textLabel.frame;
          CGFloat const padding = leftR.origin.x;
          CGRect r = CGRectMake(padding + leftR.size.width + padding, leftR.origin.y,
              cell.contentView.frame.size.width - 3 * padding - leftR.size.width, leftR.size.height);
          UITextField * f = [[[UITextField alloc] initWithFrame:r] autorelease];
          f.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
          f.enablesReturnKeyAutomatically = YES;
          f.returnKeyType = UIReturnKeyDone;
          f.clearButtonMode = UITextFieldViewModeWhileEditing;
          f.autocorrectionType = UITextAutocorrectionTypeNo;
          f.textAlignment = UITextAlignmentRight;
          f.textColor = cell.detailTextLabel.textColor;
          f.font = [cell.detailTextLabel.font fontWithSize:[cell.detailTextLabel.font pointSize]];
          f.tag = TEXTFIELD_TAG;
          f.delegate = self;
          f.autocapitalizationType = UITextAutocapitalizationTypeWords;
          // Reset temporary font
          cell.detailTextLabel.text = nil;
          [cell.contentView addSubview:f];
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
        ((UITextField *)[cell.contentView viewWithTag:TEXTFIELD_TAG]).text = m_balloon.title;
        break;

      case 1:
        cell.detailTextLabel.text = m_balloon.setName;
        break;

      case 2:
        // Create a copy of view here because it can't be subview in map view and in a cell simultaneously
        cell.accessoryView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:m_balloon.color]] autorelease];
        break;
    }
  }
  else if (indexPath.section == 1 && [self canShare])
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"SharePin"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"SharePin"] autorelease];
      cell.textLabel.textAlignment = UITextAlignmentCenter;
      cell.textLabel.text = NSLocalizedString(@"share", nil);
    }
  }
  else
  {
    // 2nd section with add/remove pin buttons
    cell = [tableView dequeueReusableCellWithIdentifier:@"removePinCellId"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"removePinCellId"] autorelease];
      cell.textLabel.textAlignment = UITextAlignmentCenter;
      cell.textLabel.text = NSLocalizedString(@"remove_pin", @"Place Page - Remove Pin button");
    }
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
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];

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
  else if (indexPath.section == 1 && ([self canShare]))
  {
    UIActionSheet * as = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"share", nil) delegate:self cancelButtonTitle:nil  destructiveButtonTitle:nil otherButtonTitles:nil];
    if ([MFMessageComposeViewController canSendText])
      [as addButtonWithTitle:NSLocalizedString(@"message", nil)];
    if ([MFMailComposeViewController canSendMail])
      [as addButtonWithTitle:NSLocalizedString(@"email", nil)];
    [as addButtonWithTitle:NSLocalizedString(@"cancel", nil)];
    [as setCancelButtonIndex:as.numberOfButtons - 1];
    [as showInView:self.view];
    [as release];
  }
  else
  {
    // Remove pin
    [self onRemoveClicked];
    m_removePinOnClose = YES;
    // Close place page
    [self.navigationController popViewControllerAnimated:YES];
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;

  // Hide keyboard
  [textField resignFirstResponder];

  if (![textField.text isEqualToString:m_balloon.title])
  {
    m_balloon.title = textField.text;
    self.navigationController.title = textField.text;
  }
  return NO;
}

- (void)actionSheet:(UIActionSheet *)actionSheet willDismissWithButtonIndex:(NSInteger)buttonIndex
{
  UITableViewCell * cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
  UITextField * textF = (UITextField *)[cell viewWithTag:TEXTFIELD_TAG];
  if ([[actionSheet buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"email", nil)])
  {
    NSString * shortUrl = [self generateShortUrlwithName:textF.text];
    [self sendEmailWith:textF.text andUrl:shortUrl];
  }
  else if ([[actionSheet buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"message", nil)])
  {
    NSString * shortUrl = [self generateShortUrlwithName:@""];
    [self sendMessageWith:textF.text andUrl:shortUrl];
  }
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
  [self dismissModalViewControllerAnimated:YES];
}

-(void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
  [self dismissModalViewControllerAnimated:YES];
}

-(void)sendEmailWith:(NSString *)textFieldText andUrl:(NSString *)shortUrl
{
  MFMailComposeViewController * mailVC = [[[MFMailComposeViewController alloc] init] autorelease];
  //TODO create subject
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];

  if ([textFieldText isEqualToString:NSLocalizedString(@"my_position", nil)])
  {
    [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_email", nil), textFieldText, shortUrl, httpGe0Url] isHTML:NO];
    [mailVC setSubject:NSLocalizedString(@"my_position_share_email_subject", nil)];
  }
  else
  {
    [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"bookmark_share_email", nil), textFieldText, shortUrl, httpGe0Url] isHTML:NO];
    [mailVC setSubject:NSLocalizedString(@"bookmark_share_email_subject", nil)];
  }

  mailVC.mailComposeDelegate = self;
  [self presentModalViewController:mailVC animated:YES];
}

-(void)sendMessageWith:(NSString *)textFieldText andUrl:(NSString *)shortUrl
{
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
  MFMessageComposeViewController * messageVC = [[[MFMessageComposeViewController alloc] init] autorelease];

  if ([textFieldText isEqualToString:NSLocalizedString(@"my_position", nil)])
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_sms", nil), shortUrl, httpGe0Url]];
  else
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"bookmark_share_sms", nil), shortUrl, httpGe0Url]];

  messageVC.messageComposeDelegate = self;
  [self presentModalViewController:messageVC animated:YES];
}

-(BOOL)canShare
{
  return [MFMessageComposeViewController canSendText] || [MFMailComposeViewController canSendMail];
}

-(NSString *)generateShortUrlwithName:(NSString *) name
{
  Framework & f = GetFramework();
  return [NSString stringWithUTF8String:(f.CodeGe0url(MercatorBounds::YToLat(m_balloon.globalPosition.y),
                                         MercatorBounds::XToLon(m_balloon.globalPosition.x),
                                         f.GetBmCategory(m_balloon.editedBookmark.first)->GetBookmark(m_balloon.editedBookmark.second)->GetScale(),
                                         [name UTF8String])).c_str()];
}

@end
