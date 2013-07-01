#import "PlacePageVC.h"
#import "SelectSetVC.h"
#import "Statistics.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "TwoButtonsView.h"
#import "ShareActionSheet.h"
#import "PlaceAndCompasView.h"

#include "Framework.h"
#include "../../search/result.hpp"
#include "../../platform/settings.hpp"


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

//Enable accessibility for this view
- (BOOL)isAccessibilityElement
{
	return YES;
}

- (NSString *)accessibilityLabel
{
	return self.titleLabel.text;
}

@end

#define TEXTFIELD_TAG 999
#define TEXTVIEW_TAG 666
#define COORDINATE_TAG 333
#define MARGIN 20
#define SMALLMARGIN 10
#define DESCRIPTIONHEIGHT 140
#define TWOBUTTONSHEIGHT 44
#define CELLHEIGHT  44
#define COORDINATECOLOR 51.0/255.0

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

typedef enum {Editing, Saved} Mode;

@interface PlacePageVC()
{
  int m_selectedRow;
  Mode m_mode;
  size_t m_categoryIndex;

  //statistics purpose
  size_t m_categoryIndexStatistics;
  size_t m_numberOfCategories;
}

@property (nonatomic, copy) UIView * viewWithPicker;

@property(nonatomic, copy) NSString * pinTitle;
// Currently displays bookmark description (notes)
@property(nonatomic, copy) NSString * pinNotes;
// Stores displayed bookmark icon file name
@property(nonatomic, retain) NSString * pinColor;
// If we clicked already existing bookmark, it will be here
@property(nonatomic, assign) BookmarkAndCategory pinEditedBookmark;

@property(nonatomic, assign) CGPoint pinGlobalPosition;
@property(nonatomic, copy) NSString * pinType;
@property (nonatomic, retain) NSArray * pinsArray;

@property (nonatomic, retain) PlaceAndCompasView * placeAndCompass;
@end

@implementation PlacePageVC

- (id) initWithInfo:(search::AddressInfo const &)info point:(CGPoint)point
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    char const * bestType = info.GetBestType();
    char const * pinName = info.GetPinName().c_str();
    [self initializeProperties:[NSString stringWithUTF8String:pinName ? pinName : ""]
                         notes:@""
                         color:@"placemark-red"
                      category:MakeEmptyBookmarkAndCategory() point:point
                          type:[NSString stringWithUTF8String: bestType ? bestType : ""]];
    m_mode = Editing;
    [self createPinPickerArray];
  }
  return self;
}

- (id) initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_mode = Editing;
    [self initializeProperties:[NSString stringWithUTF8String:apiPoint.m_name.c_str()]
                         notes:@""
                         color:@"placemark-red"
                      category:MakeEmptyBookmarkAndCategory()
                         point:CGPointMake(MercatorBounds::LonToX(apiPoint.m_lon), MercatorBounds::LatToY(apiPoint.m_lat))
                          type:@""];
    [self createPinPickerArray];
  }
  return self;
}

- (id) initWithBookmark:(BookmarkAndCategory)bmAndCat
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(bmAndCat.first);
    Bookmark * bm = cat->GetBookmark(bmAndCat.second);
    search::AddressInfo info;

    CGPoint const pt = CGPointMake(bm->GetOrg().x, bm->GetOrg().y);
    GetFramework().GetAddressInfoForGlobalPoint(bm->GetOrg(), info);


    char const * c = info.GetBestType();

    [self initializeProperties:[NSString stringWithUTF8String:bm->GetName().c_str()]
                         notes:[NSString stringWithUTF8String:bm->GetDescription().c_str()]
                         color:[NSString stringWithUTF8String:bm->GetType().c_str()]
                      category:bmAndCat
                         point:CGPointMake(pt.x, pt.y)
                          type:[NSString stringWithUTF8String: c ? c : ""]];

    m_mode = Saved;
    [self createPinPickerArray];
  }
  return self;
}

- (id) initWithName:(NSString *)name andGlobalPoint:(CGPoint)point
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_mode = Editing;
    [self initializeProperties:name
                         notes:@""
                         color:@"placemark-red"
                      category:MakeEmptyBookmarkAndCategory()
                         point:point
                          type:@""];
    [self createPinPickerArray];
  }
  return self;
}

- (void) dealloc
{
  self.pinsArray = nil;
  self.viewWithPicker = nil;

  self.pinTitle = nil;
  self.pinNotes = nil;
  self.pinColor = nil;
  self.pinType = nil;
  self.placeAndCompass = nil;
  [super dealloc];
}

- (void)viewWillAppear:(BOOL)animated
{
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  // Update the table - we can display it after changing set or color
  [self.tableView reloadData];

  // Automatically show keyboard if bookmark has default name
  if ([_pinTitle isEqualToString:NSLocalizedString(@"dropped_pin", nil)])
    [[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]].contentView viewWithTag:TEXTFIELD_TAG] becomeFirstResponder];

  if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
  {
    CGSize size = CGSizeMake(320, 480);
    self.contentSizeForViewInPopover = size;
  }
  [super viewWillAppear:animated];
}

-(void)viewDidLoad
{
  [super viewDidLoad];
  self.title = self.pinTitle;
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:UIDeviceOrientationDidChangeNotification  object:nil];
  if (m_mode == Editing)
    [self addRightNavigationItemWithAction:@selector(save)];
  else
    [self addRightNavigationItemWithAction:@selector(edit)];
}

-(void)viewDidUnload
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
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
  if (m_mode == Editing)
    switch (section)
    {
      //name
      case 0: return 1;
      //set
      case 1: return 1;
      //color picker
      case 2: return 1;
      //description
      case 3: return 1;
    }
  else
    switch (section)
    {
      //return zero, because want to use headers and footers
      //coordinates cell
      case 0: return 1;
      case 1: return 0;
      case 2: return 0;
    }
  return 0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == 3 && m_mode == Editing)
    return DESCRIPTIONHEIGHT;
  return CELLHEIGHT;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (section == 0 && m_mode == Saved)
    return self.placeAndCompass;
  if (section == 1 && [self.pinNotes length] && m_mode == Saved)
  {
    //Refactor we can do better
    CGRect z = CGRectMake(0, 0, self.tableView.frame.size.width, [self getDescriptionHeight] + MARGIN);
    UIView * view = [[[UIView alloc] initWithFrame:z] autorelease];
    z.origin.x = SMALLMARGIN;
    z.size.width -= SMALLMARGIN;
    UITextView * textView = [[UITextView alloc] initWithFrame:z];
    textView.scrollEnabled = NO;
    textView.text = self.pinNotes;
    textView.backgroundColor = [UIColor clearColor];
    textView.font = [UIFont fontWithName:@"Helvetica" size:18];
    textView.editable = NO;
    [view addSubview:textView];
    [textView release];
    return view;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if (section == 0 && m_mode == Saved)
  {
    if (!_placeAndCompass)
      _placeAndCompass = [[PlaceAndCompasView alloc] initWithName:self.pinTitle placeSecondaryName:[NSString stringWithUTF8String:GetFramework().GetBmCategory(_pinEditedBookmark.first)->GetName().c_str()] placeGlobalPoint:_pinGlobalPosition width:self.tableView.frame.size.width];
    return self.placeAndCompass.frame.size.height;
  }
  if (section == 1 && [self.pinNotes length] && m_mode == Saved)
    return [self getDescriptionHeight] + MARGIN;
  return 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  if (m_mode == Saved && section == 1)
    return [[[TwoButtonsView alloc] initWithFrame:CGRectMake(0, 0, self.tableView.frame.size.width, TWOBUTTONSHEIGHT) leftButtonSelector:@selector(share) rightButtonSelector:@selector(remove) leftButtonTitle:NSLocalizedString(@"share", nil) rightButtontitle:NSLocalizedString(@"remove_pin", nil) target:self] autorelease];
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  if (section == 1  && m_mode == Saved)
    return TWOBUTTONSHEIGHT;
  return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if (m_mode == Editing)
    cell = [self cellForEditingModeWithTable:tableView cellForRowAtIndexPath:indexPath];
  else
    cell = [self cellForSaveModeWithTable:tableView cellForRowAtIndexPath:indexPath];
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (m_mode == Editing)
  {
    if (indexPath.section == 1)
    {
      SelectSetVC * vc = [[[SelectSetVC alloc] initWithIndex:&m_categoryIndex] autorelease];
      [self pushToNavigationControllerAndSetControllerToPopoverSize:vc];
    }
    else if (indexPath.section == 2)
    {
      [self.view endEditing:YES];
      [self createAndShowColorPicker];
    }
    return;
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if (textField.text.length == 0)
    return YES;
  // Hide keyboard
  [textField resignFirstResponder];

  if (![textField.text isEqualToString:_pinTitle])
  {
    self.pinTitle = textField.text;
    self.navigationController.title = textField.text;
  }
  return NO;
}

- (void)actionSheet:(UIActionSheet *)actionSheet willDismissWithButtonIndex:(NSInteger)buttonIndex
{
  [ShareActionSheet resolveActionSheetChoice:actionSheet buttonIndex:buttonIndex text:self.pinTitle view:self delegate:self scale:GetFramework().GetDrawScale() gX:_pinGlobalPosition.x gY:_pinGlobalPosition.y andMyPosition:NO];
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
	return [_pinsArray count];
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	return 1;
}

#pragma mark - UIPickerViewDelegate
- (UIView *)pickerView:(UIPickerView *)pickerView viewForRow:(NSInteger)row
          forComponent:(NSInteger)component reusingView:(UIView *)view
{
	return [_pinsArray objectAtIndex:row];
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
  m_selectedRow = row;
}

-(void)pickerDoneClicked
{
  if (![_pinColor isEqualToString:g_colors[m_selectedRow]])
  {
    [[Statistics instance] logEvent:@"Select Bookmark color"];
    self.pinColor = g_colors[m_selectedRow];
    if (!IsValid(self.pinEditedBookmark))
      [[Statistics instance] logEvent:@"New Bookmark Color Changed"];
    [self.tableView reloadData];
  }
  [self pickerCancelClicked];
}

-(void)pickerCancelClicked
{
  self.tableView.scrollEnabled = YES;
  [_viewWithPicker removeFromSuperview];
  self.viewWithPicker = nil;
}

-(void)createAndShowColorPicker
{
  double height, width;
  CGRect rect;
  [self getSuperView:height width:width rect:rect];

  _viewWithPicker = [[UIView alloc] initWithFrame:rect];
  UIPickerView * picker = [[UIPickerView alloc] initWithFrame:CGRectMake(0.0, 0.0, 0.0, 0.0)];
  picker.delegate = self;
  picker.showsSelectionIndicator = YES;
  CGRect r = picker.frame;
  [picker setCenter:CGPointMake(width / 2, height - r.size.height / 2)];
  [_viewWithPicker addSubview:picker];

  UIToolbar * pickerToolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, picker.frame.origin.y - CELLHEIGHT, width, CELLHEIGHT)];
  pickerToolbar.barStyle = UIBarStyleBlackOpaque;

  NSMutableArray *barItems = [[NSMutableArray alloc] init];
  UIBarButtonItem *doneBtn = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(pickerDoneClicked)];
  UIBarButtonItem *cancelBtn = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(pickerCancelClicked)];
  [barItems addObject:doneBtn];
  [barItems addObject:cancelBtn];

  [pickerToolbar setItems:barItems animated:YES];
  [barItems release];

  [picker release];
  [_viewWithPicker addSubview:pickerToolbar];
  [pickerToolbar release];
  if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad)
  {
    UIWindow* window = [UIApplication sharedApplication].keyWindow;
    if (!window)
      window = [[UIApplication sharedApplication].windows objectAtIndex:0];
    [[[window subviews] objectAtIndex:0] addSubview:_viewWithPicker];
  }
  else
    [self.view.superview addSubview:_viewWithPicker];
  self.tableView.scrollEnabled = NO;
  m_selectedRow = 0;
}

-(void)getSuperView:(double &)height width:(double &)width rect:(CGRect &)rect
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

-(void)addRightNavigationItemWithAction:(SEL)selector
{
  UIBarButtonItem * but;
  if (m_mode == Saved)
    but = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemEdit target:self action:selector];
  else
    but = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave target:self action:selector];
  self.navigationItem.rightBarButtonItem = but;
  [but release];
}

-(void)save
{
  [self.view endEditing:YES];
  m_mode = Saved;
  [self savePin];
  [self goToTheMap];
  GetFramework().GetBalloonManager().Hide();
}

-(void)edit
{
  m_mode = Editing;
  [self.tableView reloadData];
  [self addRightNavigationItemWithAction:@selector(save)];
}

-(void)remove
{
  if (IsValid(_pinEditedBookmark))
  {
    BookmarkCategory * cat = GetFramework().GetBmCategory(_pinEditedBookmark.first);
    if (cat->GetBookmarksCount() > _pinEditedBookmark.second)
    {
      cat->DeleteBookmark(_pinEditedBookmark.second);
      cat->SaveToKMLFile();
    }
  }
  [self goToTheMap];
}

-(void)share
{
  [ShareActionSheet showShareActionSheetInView:self.view withObject:self];
}

-(void)createPinPickerArray
{
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

-(void)savePin
{
  Framework & f = GetFramework();
  if (_pinEditedBookmark == MakeEmptyBookmarkAndCategory())
  {
    if ([self.pinNotes length] != 0)
      [[Statistics instance] logEvent:@"New Bookmark Description Field Occupancy" withParameters:@{@"Occupancy" : @"Filled"}];
    else
      [[Statistics instance] logEvent:@"New Bookmark Description Field Occupancy" withParameters:@{@"Occupancy" : @"Empty"}];

    if (m_categoryIndexStatistics != m_categoryIndex)
      [[Statistics instance] logEvent:@"New Bookmark Category" withParameters:@{@"Changed" : @"YES"}];
    else
      [[Statistics instance] logEvent:@"New Bookmark Category" withParameters:@{@"Changed" : @"NO"}];

    if (m_numberOfCategories != GetFramework().GetBmCategoriesCount())
      [[Statistics instance] logEvent:@"New Bookmark Category Set Was Created" withParameters:@{@"Created" : @"YES"}];
    else
      [[Statistics instance] logEvent:@"New Bookmark Category Set Was Created" withParameters:@{@"Created" : @"NO"}];

    int value = 0;
    if (Settings::Get("NumberOfBookmarksPerSession", value))
      Settings::Set("NumberOfBookmarksPerSession", ++value);
    [self addBookmarkToCategory:m_categoryIndex];
  }
  else
  {
    BookmarkCategory * cat = f.GetBmCategory(_pinEditedBookmark.first);
    Bookmark * bm = cat->GetBookmark(_pinEditedBookmark.second);

    if ([self.pinColor isEqualToString:[NSString stringWithUTF8String:bm->GetType().c_str()]])
      [[Statistics instance] logEvent:@"Bookmark Color" withParameters:@{@"Changed" : @"NO"}];
    else
      [[Statistics instance] logEvent:@"Bookmark Color" withParameters:@{@"Changed" : @"YES"}];

    if ([self.pinNotes isEqualToString:[NSString stringWithUTF8String:bm->GetDescription().c_str()]])
      [[Statistics instance] logEvent:@"Bookmark Description Field" withParameters:@{@"Changed" : @"NO"}];
    else
      [[Statistics instance] logEvent:@"Bookmark Description Field" withParameters:@{@"Changed" : @"YES"}];
    if (_pinEditedBookmark.first != m_categoryIndex)
    {
      [[Statistics instance] logEvent:@"Bookmark Category" withParameters:@{@"Changed" : @"YES"}];
      cat->DeleteBookmark(_pinEditedBookmark.second);
      cat->SaveToKMLFile();
      [self addBookmarkToCategory:m_categoryIndex];
    }
    else
    {
      [[Statistics instance] logEvent:@"Bookmark Category" withParameters:@{@"Changed" : @"NO"}];
      bm->SetName([self.pinTitle UTF8String]);
      bm->SetType([self.pinColor UTF8String]);
      bm->SetDescription([self.pinNotes UTF8String]);
      cat->SaveToKMLFile();
    }
  }
}

-(void)addBookmarkToCategory:(size_t)index
{
  BookmarkCategory * cat = GetFramework().GetBmCategory(index);
  Bookmark bm(m2::PointD(_pinGlobalPosition.x, _pinGlobalPosition.y), [self.pinTitle UTF8String], [self.pinColor UTF8String]);
  bm.SetDescription([self.pinNotes UTF8String]);
  cat->AddBookmark(bm);
  cat->SaveToKMLFile();
  _pinEditedBookmark = pair<int, int>(index, cat->GetBookmarksCount() - 1);
}

-(void)initializeProperties:(NSString *)name notes:(NSString *)notes color:(NSString *)color category:(BookmarkAndCategory) bmAndCat point:
     (CGPoint)point type:(NSString *)type
{
  self.pinTitle = name;
  self.pinNotes = notes;
  self.pinColor = color;
  self.pinEditedBookmark =bmAndCat;
  m_categoryIndex = bmAndCat.first == - 1 ? GetFramework().LastEditedCategory():bmAndCat.first;
  self.pinGlobalPosition = point;
  self.pinType = type;
  m_categoryIndexStatistics = m_categoryIndex;
  m_numberOfCategories = GetFramework().GetBmCategoriesCount();
}

-(UITextView *)createTextFieldForCell:(UIFont *)font color:(UIColor *)color
{
  UITextView * txtView = [[[UITextView alloc] initWithFrame:CGRectMake(0.0, 0.0, 320.0, 142.0)] autorelease];
  txtView.delegate = self;
  txtView.AutoresizingMask = UIViewAutoresizingFlexibleWidth;
  txtView.textColor = color;
  txtView.font = font;
  txtView.backgroundColor = [UIColor clearColor];
  txtView.tag = TEXTVIEW_TAG;
  return txtView;
}

-(UITableViewCell *)cellForEditingModeWithTable:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * cellId;
  switch (indexPath.section)
  {
    case 0: cellId = @"EditingNameCellId"; break;
    case 1: cellId = @"EditingSetCellId"; break;
    case 2: cellId = @"EditingColorCellId"; break;
    default: cellId = @"EditingDF"; break;
  }
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:cellId];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:cellId] autorelease];
    if (indexPath.section == 0)
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
      [f addTarget:self action:@selector(textFieldDidChange:) forControlEvents:UIControlEventEditingChanged];
      // Reset temporary font
      cell.detailTextLabel.text = nil;
      [cell.contentView addSubview:f];
    }
    else if (indexPath.section == 1)
    {
      cell.textLabel.text = NSLocalizedString(@"set", @"Add bookmark dialog - bookmark set");
      cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    else if (indexPath.section == 2)
    {
      cell.textLabel.text = NSLocalizedString(@"color", @"Add bookmark dialog - bookmark color");
    }
    else if (indexPath.section == 3)
    {
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
      // Temporary, to init font and color
      cell.detailTextLabel.text = @"temp string";
      // Called to initialize frames and fonts
      [cell layoutSubviews];
      UITextView * txtView = [[[UITextView alloc] initWithFrame:CGRectMake(0.0, 0.0, 320.0, 142.0)] autorelease];
      txtView.delegate = self;
      txtView.AutoresizingMask = UIViewAutoresizingFlexibleWidth;
      txtView.textColor = cell.detailTextLabel.textColor;
      txtView.font = [cell.detailTextLabel.font fontWithSize:[cell.detailTextLabel.font pointSize]];
      txtView.backgroundColor = [UIColor clearColor];
      txtView.tag = TEXTVIEW_TAG;
      cell.detailTextLabel.text = @"";
      [cell.contentView addSubview:txtView];
      txtView.delegate = self;
    }
  }
  switch (indexPath.section)
  {
    case 0:
      ((UITextField *)[cell.contentView viewWithTag:TEXTFIELD_TAG]).text = self.pinTitle;
      break;

    case 1:
      cell.detailTextLabel.text = [NSString stringWithUTF8String:GetFramework().GetBmCategory(m_categoryIndex)->GetName().c_str()];
      break;

    case 2:
      cell.accessoryView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:_pinColor]] autorelease];
      break;
    case 3:
      UITextView * t = (UITextView *)[cell viewWithTag:TEXTVIEW_TAG];
      t.text = self.pinNotes;
      break;
  }
  return cell;
}

-(UITableViewCell *)cellForSaveModeWithTable:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if (indexPath.section == 0)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"CoordinatesCELL"];
    if (!cell)
    {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"CoordinatesCELL"] autorelease];
      cell.textLabel.textAlignment = UITextAlignmentCenter;
      cell.textLabel.font = [UIFont fontWithName:@"Helvetica" size:26];
      cell.textLabel.textColor = [UIColor colorWithRed:COORDINATECOLOR green:COORDINATECOLOR blue:COORDINATECOLOR alpha:1.0];
      UILongPressGestureRecognizer * longTouch = [[[UILongPressGestureRecognizer alloc]
                                            initWithTarget:self action:@selector(handleLongPress:)] autorelease];
      longTouch.minimumPressDuration = 1.0;
      longTouch.delegate = self;
      [cell addGestureRecognizer:longTouch];
    }
    cell.textLabel.text = [self coordinatesToString];
  }
  return cell;
}

-(void)orientationChanged
{
  if (m_mode == Saved)
  {
    [self.placeAndCompass drawView];
    [self.tableView reloadData];
  }
}

-(void)goToTheMap
{
  GetFramework().GetBalloonManager().Hide();
  if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    [[MapsAppDelegate theApp].m_mapViewController dismissPopover];
  else
    [self.navigationController popToRootViewControllerAnimated:YES];
}

-(CGFloat)getDescriptionHeight
{
  return [self.pinNotes sizeWithFont:[UIFont fontWithName:@"Helvetica" size:18] constrainedToSize:CGSizeMake(self.tableView.frame.size.width - 3 * SMALLMARGIN, CGFLOAT_MAX) lineBreakMode:NSLineBreakByCharWrapping].height;
}

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer
{
  if (gestureRecognizer.state == UIGestureRecognizerStateBegan)
  {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    NSIndexPath * indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil)
    {
      [self becomeFirstResponder];
      UIMenuController * menu = [UIMenuController sharedMenuController];
      [menu setTargetRect:[self.tableView rectForRowAtIndexPath:indexPath] inView:self.tableView];
      [menu setMenuVisible:YES animated:YES];
    }
  }
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
  if (action == @selector(copy:))
    return YES;
  return NO;
}

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (void)copy:(id)sender
{
  [UIPasteboard generalPasteboard].string = [self coordinatesToString];
}

-(NSString *)coordinatesToString
{
  NSLocale * decimalPointLocale = [[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"] autorelease];
  return [[[NSString alloc] initWithFormat:@"%@" locale:decimalPointLocale, [NSString stringWithFormat:@"%f %f", MercatorBounds::YToLat(self.pinGlobalPosition.y), MercatorBounds::XToLon(self.pinGlobalPosition.x)]] autorelease];
}

-(void)textFieldDidChange:(UITextField *)textField
{
  if (textField.tag == TEXTFIELD_TAG)
    self.pinTitle = textField.text;
}

- (void)textViewDidChange:(UITextView *)textView
{
  if (textView.tag == TEXTVIEW_TAG)
    self.pinNotes = textView.text;
}

@end
