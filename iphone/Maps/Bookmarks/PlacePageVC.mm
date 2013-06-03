#import "PlacePageVC.h"
#import "BalloonView.h"
#import "SelectSetVC.h"
#import "EditDescriptionVC.h"
#import "Statistics.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"

@interface PinPickerView : UIView

- (id)initWithImage:(UIImage *)image;

+ (CGFloat)viewWidth;
+ (CGFloat)viewHeight;

@end

@interface PinPickerView ()
@property (nonatomic, retain) UILabel * titleLabel;
@end

@implementation PinPickerView

CGFloat const pWidth = 200;
CGFloat const pHeight = 36;
CGFloat const pMargin = 10;

+ (CGFloat)viewWidth
{
  return pWidth;
}

+ (CGFloat)viewHeight
{
  return pHeight;
}

- (id)initWithImage:(UIImage *)image
{
  self = [super initWithFrame:CGRectMake(0.0, 0.0, pWidth, pHeight)];
	if (self)
	{
		CGFloat y = (self.bounds.size.height - image.size.height) / 2;
    UIImageView * imageView = [[UIImageView alloc] initWithFrame:
                              CGRectMake(pMargin,
                                         y,
                                         image.size.width,
                                         image.size.height)];
    imageView.image = image;
    [self addSubview:imageView];
    [imageView release];
	}
	return self;
}

// Enable accessibility for this view.
- (BOOL)isAccessibilityElement
{
	return YES;
}

// Return a string that describes this view.
- (NSString *)accessibilityLabel
{
	return self.titleLabel.text;
}

@end

#define TEXTFIELD_TAG 999

static NSString  * const g_colors [] =
{
  @"placemark-red",
  @"placemark-blue",
  @"placemark-brown",
  @"placemark-green",
  @"placemark-orange",
  @"placemark-pink",
  @"placemark-purple"
};

@interface PlacePageVC()
@property (nonatomic, retain) UIView * viewWithPicker;
@end

@implementation PlacePageVC

@synthesize pinsArray, viewWithPicker;

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_balloon = view;
    self.title = m_balloon.title;
    m_balloon.isCurrentPosition = NO;

    NSMutableArray * viewArray = [[NSMutableArray alloc] init];

    for (size_t i = 0; i < ARRAY_SIZE(g_colors); ++i)
    {

      PinPickerView * pinView = [[PinPickerView alloc] initWithImage:[UIImage imageNamed:[NSString stringWithFormat:@"%@.png", g_colors[i]]]];
		  [viewArray addObject:pinView];
      [pinView release];
    }
		self.pinsArray = viewArray;
    [viewArray release];
  }
  return self;
}

- (void) dealloc
{
  [pinsArray release];
  [viewWithPicker release];
  [super dealloc];
}

- (void)viewWillAppear:(BOOL)animated
{
  m_hideNavBar = YES;
  self.navigationController.navigationBar.barStyle = UIBarStyleDefault;
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  // Update the table - we can display it after changing set or color
  [self.tableView reloadData];

  // Should be set to YES only if Remove Pin was pressed
  m_removePinOnClose = NO;

  // Automatically show keyboard if bookmark has default name
  if ([m_balloon.title isEqualToString:NSLocalizedString(@"dropped_pin", nil)])
    [[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]].contentView viewWithTag:TEXTFIELD_TAG] becomeFirstResponder];

  if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
  {
    CGSize size = CGSizeMake(320, 480);
    self.contentSizeForViewInPopover = size;
  }

  [[NSNotificationCenter defaultCenter] addObserver:self  selector:@selector(orientationChanged)  name:UIDeviceOrientationDidChangeNotification  object:nil];

  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  if (m_hideNavBar && ![[MapsAppDelegate theApp].m_mapViewController shouldShowNavBar])
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
    if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad && [self.navigationController.viewControllers indexOfObject:self] == NSNotFound)
    {
      [m_balloon addOrEditBookmark];
      [m_balloon clear];
    }
  }
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super viewWillDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 4;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  switch (section)
  {
  case 0: return 3;
  case 1: return 1;
  case 2: return 1;
  case 3: return 1;
  default: return 0;
  }
}

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
  else if (indexPath.section == 1)
  {
    NSString * cellLabelText = NSLocalizedString(@"description", nil);
    cell = [tableView dequeueReusableCellWithIdentifier:cellLabelText];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:cellLabelText] autorelease];
      cell.textLabel.text = cellLabelText;
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    cell.detailTextLabel.text = m_balloon.notes;
  }
  else if (indexPath.section == 2)
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
  [m_balloon clear];
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
        [self pushToNavigationControllerAndSetControllerToPopoverSize:vc];
        [vc release];
      }
      break;

    case 2:
      {
        [self createAndShowColorPicker];
      }
      break;
    }
  }
  else if (indexPath.section == 1)
  {
    m_hideNavBar = NO;
    EditDescriptionVC * vc = [[EditDescriptionVC alloc] initWithBalloonView:m_balloon];
    [self pushToNavigationControllerAndSetControllerToPopoverSize:vc];
    [vc release];
  }
  else if (indexPath.section == 2)
  {
    UIActionSheet * as = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"share", nil) delegate:self cancelButtonTitle:nil  destructiveButtonTitle:nil otherButtonTitles:nil];
    if ([MFMessageComposeViewController canSendText])
      [as addButtonWithTitle:NSLocalizedString(@"message", nil)];
    if ([MFMailComposeViewController canSendMail])
      [as addButtonWithTitle:NSLocalizedString(@"email", nil)];
    [as addButtonWithTitle:NSLocalizedString(@"copy_link", nil)];
    [as addButtonWithTitle:NSLocalizedString(@"cancel", nil)];
    [as setCancelButtonIndex:as.numberOfButtons - 1];
    [as showInView:self.view];
    [as release];
  }
  else
  {
    m_removePinOnClose = YES;
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
      [[MapsAppDelegate theApp].m_mapViewController dismissPopoverAndSaveBookmark:NO];
    else
    {
      // Remove pin
      [self onRemoveClicked];
      [self.navigationController popViewControllerAnimated:YES];
    }
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
    // Not really beutiful now, but ...
    BOOL isMyPosition = [textF.text isEqualToString:NSLocalizedString(@"my_position", nil)];
    NSString * shortUrl = [self generateShortUrlwithName:isMyPosition ? @"" : textF.text];

    [self sendEmailWith:textF.text andUrl:shortUrl];
  }
  else if ([[actionSheet buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"message", nil)])
  {
    NSString * shortUrl = [self generateShortUrlwithName:@""];
    [self sendMessageWith:textF.text andUrl:shortUrl];
  }
  else  if ([[actionSheet buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"copy_link", nil)])
  {
    [UIPasteboard generalPasteboard].string = [self generateShortUrlwithName:textF.text];
  }
}

- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error
{
  [[Statistics instance] logEvent:@"ge0(zero) MAIL Export"];
  [self dismissModalViewControllerAnimated:YES];
}

-(void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
  [[Statistics instance] logEvent:@"ge0(zero) MESSAGE Export"];
  [self dismissModalViewControllerAnimated:YES];
}

-(void)sendEmailWith:(NSString *)textFieldText andUrl:(NSString *)shortUrl
{
  MFMailComposeViewController * mailVC = [[[MFMailComposeViewController alloc] init] autorelease];
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];

  if ([textFieldText isEqualToString:NSLocalizedString(@"my_position", nil)])
  {
    m2::PointD pt = m2::PointD(m_balloon.globalPosition.x, m_balloon.globalPosition.y);
    NSString * nameAndAddress = [NSString stringWithUTF8String:GetFramework().GetNameAndAddressAtGlobalPoint(pt).c_str()];
    [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_email", nil), nameAndAddress, shortUrl, httpGe0Url] isHTML:NO];
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

-(void)pushToNavigationControllerAndSetControllerToPopoverSize:(UIViewController *)vc
{
  if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    [vc setContentSizeForViewInPopover:[self contentSizeForViewInPopover]];
  [self.navigationController pushViewController:vc animated:YES];
}


- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component
{
	return [PinPickerView viewWidth];
}

- (CGFloat)pickerView:(UIPickerView *)pickerView rowHeightForComponent:(NSInteger)component
{
	return [PinPickerView viewHeight];
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	return [pinsArray count];
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	return 1;
}


#pragma mark - UIPickerViewDelegate

// tell the picker which view to use for a given component and row, we have an array of views to show
- (UIView *)pickerView:(UIPickerView *)pickerView viewForRow:(NSInteger)row
          forComponent:(NSInteger)component reusingView:(UIView *)view
{
	return [pinsArray objectAtIndex:row];
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
  selectedRow = row;
}

-(void)pickerDoneClicked
{
  if (![m_balloon.color isEqualToString:g_colors[selectedRow]])
  {
    m_balloon.pinImage.image = [UIImage imageNamed:g_colors[selectedRow]];
    [[Statistics instance] logEvent:@"Select Bookmark color"];
    m_balloon.color = g_colors[selectedRow];
    [self.tableView reloadData];
  }
  [self pickerCancelClicked];
}

-(void)pickerCancelClicked
{
  self.tableView.scrollEnabled = YES;
  [viewWithPicker removeFromSuperview];
  self.viewWithPicker = nil;
}

-(void)createAndShowColorPicker
{
  double height, width;
  CGRect rect;
  [self getScreenHeight:height width:width rect:rect];

  viewWithPicker = [[UIView alloc] initWithFrame:rect];
  UIPickerView * picker = [[UIPickerView alloc] initWithFrame:CGRectMake(0.0, 0.0, 0.0, 0.0)];
  picker.delegate = self;
  picker.showsSelectionIndicator = YES;
  CGRect r = picker.frame;
  [picker setCenter:CGPointMake(width / 2, height - r.size.height / 2)];
  [viewWithPicker addSubview:picker];

  UIToolbar * pickerToolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, picker.frame.origin.y - 44, width, 44)];
  pickerToolbar.barStyle = UIBarStyleBlackOpaque;
  //[pickerToolbar sizeToFit];

  NSMutableArray *barItems = [[NSMutableArray alloc] init];
  UIBarButtonItem *doneBtn = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(pickerDoneClicked)];
  UIBarButtonItem *cancelBtn = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(pickerCancelClicked)];
  [barItems addObject:doneBtn];
  [barItems addObject:cancelBtn];

  [pickerToolbar setItems:barItems animated:YES];
  [barItems release];


  [picker release];
  [viewWithPicker addSubview:pickerToolbar];
  [pickerToolbar release];
  if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad)
  {
    UIWindow* window = [UIApplication sharedApplication].keyWindow;
    if (!window)
      window = [[UIApplication sharedApplication].windows objectAtIndex:0];
    [[[window subviews] objectAtIndex:0] addSubview:viewWithPicker];
  }
  else
  {
    [self.view.superview addSubview:viewWithPicker];
  }

  self.tableView.scrollEnabled = NO;

  selectedRow = 0;
}

-(void) orientationChanged
{
  if (viewWithPicker)
  {
    double height, width;
    CGRect rect;
    [self getScreenHeight:height width:width rect:rect];
    [viewWithPicker setFrame:rect];

    NSArray *subviews = [viewWithPicker subviews];

    UIPickerView * p = nil;
    UIToolbar * t = nil;
    for (UIView *subview in subviews)
    {
      if ([subview isKindOfClass:[UIPickerView class]])
        p = (UIPickerView *)subview;
      else if ([subview isKindOfClass:[UIToolbar class]])
        t = (UIToolbar *)subview;
    }
    [p setFrame:CGRectMake(0, height - p.frame.size.height, width, p.frame.size.height)];
    [t setFrame:CGRectMake(0, p.frame.origin.y - 44, width, 44)];
  }
}

-(void)getScreenHeight:(double &)height width:(double &)width rect:(CGRect &)rect
{
  if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad)
  {
    rect = [UIScreen mainScreen].bounds;
    height = self.view.window.frame.size.height;
    width  = self.view.window.frame.size.width;
    if(!UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
      std::swap(height, width);

  }
  else
  {
    height = self.view.superview.frame.size.height;
    width = self.view.superview.frame.size.width;
    rect = self.view.superview.frame;
  }
}

@end
