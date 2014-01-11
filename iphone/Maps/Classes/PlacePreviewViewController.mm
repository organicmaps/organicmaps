#import "PlacePreviewViewController.h"
#import "TwoButtonsView.h"
#import "PlacePageVC.h"
#import "ShareActionSheet.h"
#import "Framework.h"
#import "Statistics.h"
#import "PlaceAndCompasView.h"
#import "CompassView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMApi.h"
#import "UIKitCategories.h"

#include "../../../search/result.hpp"
#include "../../../platform/platform.hpp"

#define BALLOON_PROPOSAL_ALERT_VIEW 11
#define TWOBUTTONSHEIGHT 44
#define COORDINATE_TAG 333
#define COORDINATECOLOR 51.0/255.0

typedef enum {APIPOINT, POI, MYPOSITION} Type;

@interface PlacePreviewViewController()
{
  Type m_previewType;
  search::AddressInfo m_poiInfo;
  url_scheme::ApiPoint m_apiPoint;
  CGPoint m_point;
}
@property (nonatomic, retain) PlaceAndCompasView * placeAndCompass;
@property (retain) ShareActionSheet * shareActionSheet;

@end

@implementation PlacePreviewViewController

- (id)initWith:(search::AddressInfo const &)info point:(CGPoint)point
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_previewType = POI;
    m_poiInfo = search::AddressInfo(info);
    // @TODO Better to have common logic for empty names. Here we have separate ifs in
    // c++ balloon code, which display "Dropped Pin" label if GetPinName is empty,
    // and similar if in this place
    if (m_poiInfo.GetPinName().empty())
      m_poiInfo.m_name = [NSLocalizedString(@"dropped_pin", nil) UTF8String];
    m_point = point;
  }
  return self;
}

- (id)initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_previewType = APIPOINT;
    m_apiPoint = apiPoint;
    m_point.y = MercatorBounds::LatToY(apiPoint.m_lat);
    m_point.x = MercatorBounds::LonToX(apiPoint.m_lon);
  }
  return self;
}

- (id)initWithPoint:(CGPoint)point
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_previewType = MYPOSITION;
    m_point = point;
  }
  return self;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (isIPad)
    self.contentSizeForViewInPopover = CGSizeMake(320, 480);;
}

#pragma mark - Table view data source

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setTitle:NSLocalizedString(@"info", nil)];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:UIDeviceOrientationDidChangeNotification object:nil];
  self.tableView.backgroundView = nil;
  self.tableView.backgroundColor = [UIColor applicationBackgroundColor];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return (m_previewType == APIPOINT && [MWMApi canOpenApiUrl:m_apiPoint]) ? 3 : 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == 0)
    return 1;
  if (section == 2 && m_previewType == APIPOINT)
    return 1;
  return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if(indexPath.section == 0)
  {
    if (indexPath.row == 0)
    {
      cell = [tableView dequeueReusableCellWithIdentifier:@"CoordinatesCELL"];
      if (!cell)
      {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"CoordinatesCELL"];
        cell.textLabel.textAlignment = UITextAlignmentCenter;
        cell.textLabel.font = [UIFont fontWithName:@"Helvetica" size:26];
        cell.textLabel.textColor = [UIColor colorWithRed:COORDINATECOLOR green:COORDINATECOLOR blue:COORDINATECOLOR alpha:1.0];
        UILongPressGestureRecognizer * longTouch = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handleLongPress:)];
        longTouch.minimumPressDuration = 0.5;
        longTouch.delegate = self;
        [cell addGestureRecognizer:longTouch];
      }
      cell.textLabel.text = [self coordinatesToString];
    }
  }
  else
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"ApiReturnCell"];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"ApiReturnCell"];
      cell.textLabel.textAlignment = NSTextAlignmentCenter;
      UIButton * tmp = [UIButton buttonWithType:UIButtonTypeRoundedRect];
      [tmp setTitle:@"tmp" forState:UIControlStateNormal];
      cell.textLabel.font = tmp.titleLabel.font;
      cell.textLabel.textColor = tmp.titleLabel.textColor;
    }
    cell.textLabel.text = NSLocalizedString(@"more_info", nil);
  }
  return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (m_previewType == APIPOINT && indexPath.section == 2)
    [MWMApi openAppWithPoint:m_apiPoint];
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  if (section == 0)
    return [self getCompassView];
  return nil;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  if (section == 1)
  {
    TwoButtonsView * myView = [[TwoButtonsView alloc] initWithFrame:CGRectMake(0, 0, self.tableView.frame.size.width, TWOBUTTONSHEIGHT) leftButtonSelector:@selector(share) rightButtonSelector:@selector(addToBookmark) leftButtonTitle:NSLocalizedString(@"share", nil) rightButtontitle:NSLocalizedString(@"add_to_bookmarks", nil) target:self];
    return myView;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  if (section == 1)
    return TWOBUTTONSHEIGHT;
  return 8;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if (section == 0)
    return [self getCompassView].frame.size.height;
  return 14;
}

- (void)share
{
  string name = "";
  if (m_previewType == POI)
    name = m_poiInfo.GetPinName();
  else if (m_previewType == APIPOINT)
    name = m_apiPoint.m_name;
  BOOL const myPos = (m_previewType == MYPOSITION) ? YES : NO;

  NSString * text = [NSString stringWithUTF8String:name.c_str()];
  ShareInfo * info = [[ShareInfo alloc] initWithText:text gX:m_point.x gY:m_point.y myPosition:myPos];
  self.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self];
  [self.shareActionSheet show];
}

- (void)addToBookmark
{
  if (!GetPlatform().HasBookmarks())
  {
    // Display banner for paid version
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"bookmarks_in_pro_version", nil)
                                                     message:nil
                                                    delegate:self
                                           cancelButtonTitle:NSLocalizedString(@"cancel", nil)
                                           otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
    alert.tag = BALLOON_PROPOSAL_ALERT_VIEW;

    [alert show];
  }
  else
  {
    PlacePageVC * p = nil;
    if (m_previewType == POI)
      p = [[PlacePageVC alloc] initWithInfo:m_poiInfo point:m_point];
    else if (m_previewType == APIPOINT)
      p = [[PlacePageVC alloc] initWithApiPoint:m_apiPoint];
    else
      p = [[PlacePageVC alloc] initWithName:NSLocalizedString(@"my_position", nil) andGlobalPoint:m_point];
    [self.navigationController pushViewController:p animated:YES];
  }
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (alertView.tag == BALLOON_PROPOSAL_ALERT_VIEW)
  {
    if (buttonIndex != alertView.cancelButtonIndex)
    {
      // Launch appstore
      [[UIApplication sharedApplication] openProVersion];
      [[Statistics instance] logProposalReason:@"Balloon Touch" withAnswer:@"YES"];
    }
    else
      [[Statistics instance] logProposalReason:@"Balloon Touch" withAnswer:@"NO"];
  }
}

- (NSURL *)getBackUrl
{
  string const str = GetFramework().GenerateApiBackUrl(m_apiPoint);
  return [NSURL URLWithString:[NSString stringWithUTF8String:str.c_str()]];
}

- (void)orientationChanged
{
  [self.placeAndCompass drawView];
  [self.tableView reloadData];
}

- (void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer
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

- (NSString *)coordinatesToString
{
  NSString * result = nil;
  if (m_previewType == APIPOINT)
    result = [NSString stringWithFormat:@"%.05f %.05f", m_apiPoint.m_lat, m_apiPoint.m_lon];
  else
    result = [NSString stringWithFormat:@"%.05f %.05f", MercatorBounds::YToLat(m_point.y), MercatorBounds::XToLon(m_point.x)];
  NSLocale * decimalPointLocale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US"];
  return [[NSString alloc] initWithFormat:@"%@" locale:decimalPointLocale, result];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (PlaceAndCompasView *)getCompassView
{
  if (!self.placeAndCompass)
  {
    NSString * name = nil;
    NSString * type = @"";
    if (m_previewType == POI)
    {
      name = [NSString stringWithUTF8String:m_poiInfo.GetPinName().c_str()];
      char const * c = m_poiInfo.GetBestType();
      type = c ? [NSString stringWithUTF8String:c] : @"";
    }
    else if (m_previewType == APIPOINT)
      name = [NSString stringWithUTF8String:m_apiPoint.m_name.c_str()];
    else
      name = NSLocalizedString(@"my_position", nil);
    if (!_placeAndCompass)
      _placeAndCompass = [[PlaceAndCompasView alloc] initWithName:name placeSecondaryName:type placeGlobalPoint:m_point width:self.tableView.frame.size.width];
  }
  return self.placeAndCompass;
}

@end
