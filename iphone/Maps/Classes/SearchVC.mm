#import "SearchVC.h"
#import "CompassView.h"
#import "LocationManager.h"
#import "SearchBannerChecker.h"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"
#include "../../platform/settings.hpp"
#include "../../platform/platform.hpp"
#include "../../indexer/mercator.hpp"
#include "../../map/framework.hpp"
#include "../../search/result.hpp"

SearchVC * g_searchVC = nil;
volatile int g_queryId = 0;

@interface Wrapper : NSObject
{
  search::Result * m_result;
}
- (id)initWithResult:(search::Result const &) res;
- (search::Result const *)get;
@end

@implementation Wrapper
- (id)initWithResult:(search::Result const &) res
{
  if ((self = [super init]))
    m_result = new search::Result(res);
  return self;
}

- (void)dealloc
{
  delete m_result;
  [super dealloc];
}

- (search::Result const *)get
{
  return m_result;
}
@end

static void OnSearchResultCallback(search::Result const & res, int queryId)
{
  if (g_searchVC && queryId == g_queryId)
  {
    // end marker means that the search is finished
    if (!res.IsEndMarker())
    {
      Wrapper * w = [[Wrapper alloc] initWithResult:res];
      [g_searchVC performSelectorOnMainThread:@selector(addResult:)
                                 withObject:w
                              waitUntilDone:NO];
      [w release];
    }
  }
}

/////////////////////////////////////////////////////////////////////

@interface CustomView : UIView
@end
@implementation CustomView
- (void)layoutSubviews
{
  UINavigationBar * navBar = (UINavigationBar *)[self.subviews objectAtIndex:0];
  [navBar sizeToFit];
  [navBar.topItem.titleView sizeToFit];

  CGFloat const wAndH = navBar.topItem.titleView.frame.size.height - 8;
  navBar.topItem.leftBarButtonItem.customView.frame = CGRectMake(0, 0, wAndH, wAndH);

  UIView * table = [self.subviews objectAtIndex:1];
  CGRect rTable;
  rTable.origin = CGPointMake(navBar.frame.origin.x, navBar.frame.origin.y + navBar.frame.size.height);
  rTable.size = self.bounds.size;
  rTable.size.height -= navBar.bounds.size.height;
  table.frame = rTable;
}
@end

////////////////////////////////////////////////////////////////////
/// Key to store settings
#define RADAR_MODE_SETTINGS_KEY "SearchRadarMode"

@implementation SearchVC

- (id)initWithFramework:(Framework *)framework andLocationManager:(LocationManager *)lm
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    m_framework = framework;
    m_locationManager = lm;
  }
  return self;
}

- (void)enableRadarMode
{
  m_radarButton.selected = YES;
  // @TODO add code for search engine
  // or add additional parameter to query by checking if (m_radarButton.selected)
  // m_framework->GetSearchEngine()->
}

- (void)disableRadarMode
{
  m_radarButton.selected = NO;
  // @TODO add code for search engine
  // or add additional parameter to query by checking if (m_radarButton.selected)
  // m_framework->GetSearchEngine()->
}

- (void)onRadarButtonClicked:(id)button
{
  UIButton * btn = (UIButton *)button;
  // Button selected state will be changed inside functions
  Settings::Set(RADAR_MODE_SETTINGS_KEY, !btn.selected);
  if (!btn.selected)
    [self enableRadarMode];
  else
    [self disableRadarMode];
}

- (void)loadView
{
  // create user interface
  // Custom view is used to automatically layout all elements
  UIView * parentView = [[[CustomView alloc] init] autorelease];
  parentView.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;

  // Button will be resized in CustomView above
  m_radarButton = [[UIButton buttonWithType:UIButtonTypeCustom] retain];
  UIImage * image = [UIImage imageNamed:@"location"];
  [m_radarButton setImage:image forState:UIControlStateNormal];
  [m_radarButton setImage:[UIImage imageNamed:@"location-highlighted"] forState:UIControlStateHighlighted];
  [m_radarButton setImage:[UIImage imageNamed:@"location-selected"] forState:UIControlStateSelected];
  m_radarButton.backgroundColor = [UIColor clearColor];
  [m_radarButton addTarget:self action:@selector(onRadarButtonClicked:) forControlEvents:UIControlEventTouchUpInside];

  UINavigationBar * navBar = [[[UINavigationBar alloc] init] autorelease];
  navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  UINavigationItem * item = [[[UINavigationItem alloc] init] autorelease];
  item.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithCustomView:m_radarButton] autorelease];

  m_searchBar = [[UISearchBar alloc] init];
  m_searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  m_searchBar.delegate = self;
  m_searchBar.placeholder = NSLocalizedString(@"Search map", @"Search box placeholder text");
  m_searchBar.showsCancelButton = YES;
  item.titleView = m_searchBar;

  [navBar pushNavigationItem:item animated:NO];

  [parentView addSubview:navBar];

  m_table = [[UITableView alloc] init];
  m_table.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
  m_table.delegate = self;
  m_table.dataSource = self;
  [parentView addSubview:m_table];

  self.view = parentView;
}

- (void)clearResults
{
  m_results.clear();
}

- (void)dealloc
{
  g_searchVC = nil;
  [m_radarButton release];
  [m_searchBar release];
  [m_table release];
  [self clearResults];
  [super dealloc];
}

- (void)viewDidLoad
{
  g_searchVC = self;
}

- (void)viewDidUnload
{
  g_searchVC = nil;
  // to correctly free memory
  [m_radarButton release]; m_radarButton = nil;
  [m_searchBar release]; m_searchBar = nil;
  [m_table release]; m_table = nil;
  m_results.clear();
  
  [super viewDidUnload];
}

// Banner dialog handler
- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    // Launch appstore
    string bannerUrl;
    Settings::Get(SETTINGS_REDBUTTON_URL_KEY, bannerUrl);
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:bannerUrl.c_str()]]];
  }
  // Close Search view
  [self dismissModalViewControllerAnimated:YES];
}

- (void)viewWillAppear:(BOOL)animated
{
  // Disable search for free version
  if (!GetPlatform().IsFeatureSupported("search"))
  {
    // Hide scope bar
    m_searchBar.showsScopeBar = NO;
    // Display banner for paid version
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Search is only available in the full version of MapsWithMe. Would you like to get it now?", @"Search button pressed dialog title in the free version")
                                                     message:nil
                                                    delegate:self
                                           cancelButtonTitle:NSLocalizedString(@"Cancel", @"Search button pressed dialog Negative button in the free version")
                                           otherButtonTitles:NSLocalizedString(@"Get it now", @"Search button pressed dialog Positive button in the free version"), nil];
    [alert show];
    [alert release];
  }
  else
  {
    // load previously saved search mode
    bool radarEnabled = false;
    Settings::Get(RADAR_MODE_SETTINGS_KEY, radarEnabled);
    if (radarEnabled)
      [self enableRadarMode];
    else
      [self disableRadarMode];

    [m_locationManager start:self];

    // show keyboard
    [m_searchBar becomeFirstResponder];
  }
  
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [m_locationManager stop:self];
  
  // hide keyboard immediately
  [m_searchBar resignFirstResponder];
  
  [super viewWillDisappear:animated];
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [m_locationManager setOrientation:self.interfaceOrientation];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

//**************************************************************************
//*********** SearchBar handlers *******************************************
- (void)searchBar:(UISearchBar *)sender textDidChange:(NSString *)searchText
{
  [self clearResults];
  [m_table reloadData];
  ++g_queryId;

  if ([searchText length] > 0)
    m_framework->Search([[searchText precomposedStringWithCompatibilityMapping] UTF8String],
              bind(&OnSearchResultCallback, _1, g_queryId));
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  [self dismissModalViewControllerAnimated:YES];
}
//*********** End of SearchBar handlers *************************************
//***************************************************************************

- (void)updateCellAngle:(UITableViewCell *)cell withIndex:(NSUInteger)index andAngle:(double)northDeg
{
  CLLocation * loc = [m_locationManager lastLocation];
  if (loc)
  {
    m2::PointD const center = m_results[index].GetFeatureCenter();
    double const angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
        MercatorBounds::LatToY(loc.coordinate.latitude)), center) + northDeg / 180. * math::pi;
    
    if (m_results[index].GetResultType() == search::Result::RESULT_FEATURE)
    {
      CompassView * cv = (CompassView *)cell.accessoryView;
      if (!cv)
      {
        float const h = m_table.rowHeight * 0.6;
        cv = [[CompassView alloc] initWithFrame:CGRectMake(0, 0, h, h)];
        cell.accessoryView = cv;
        [cv release];
      }
      cv.angle = angle;
    }
  }
}

- (void)updateCellDistance:(UITableViewCell *)cell withIndex:(NSUInteger)index
{
  CLLocation * loc = [m_locationManager lastLocation];
  if (loc)
  {
    m2::PointD const center = m_results[index].GetFeatureCenter();
    double const centerLat = MercatorBounds::YToLat(center.y);
    double const centerLon = MercatorBounds::XToLon(center.x);
    double const distance = ms::DistanceOnEarth(loc.coordinate.latitude, loc.coordinate.longitude, centerLat, centerLon);

    // @TODO use imperial system from the settings if needed
    // @TODO use meters too
    // NSLocalizedString(@"%.1lf m", @"Search results - Metres")
    // NSLocalizedString(@"%.1lf ft", @"Search results - Feet")
    // NSLocalizedString(@"%.1lf mi", @"Search results - Miles")
    // NSLocalizedString(@"%.1lf yd", @"Search results - Yards")
    cell.detailTextLabel.text = [NSString stringWithFormat:NSLocalizedString(@"%.1lf km", @"Search results - Kilometres"),
                                 distance / 1000.0];
  }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_results.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"SearchVCTableViewCell"];
  if (!cell)
  {
    cell = [[[UITableViewCell alloc]
           initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"SearchVCTableViewCell"]
           autorelease];
  }
  
  cell.accessoryView = nil;
  if (indexPath.row < m_results.size())
  {
    search::Result const & r = m_results[indexPath.row];
    cell.textLabel.text = [NSString stringWithUTF8String:r.GetString()];
    if (r.GetResultType() == search::Result::RESULT_FEATURE)
      [self updateCellDistance:cell withIndex:indexPath.row];
    else
      cell.detailTextLabel.text = nil;
  }
  else
    cell.textLabel.text = @"BUG";
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row < m_results.size())
  {
    search::Result const & res = m_results[indexPath.row];
    switch(res.GetResultType())
    {
      // Zoom to the feature
    case search::Result::RESULT_FEATURE:
      m_framework->ShowRect(res.GetFeatureRect());
      [self searchBarCancelButtonClicked:m_searchBar];
      break;

    case search::Result::RESULT_SUGGESTION:
      char const * s = res.GetSuggestionString();
      [m_searchBar setText: [NSString stringWithUTF8String:s]];
      break;
    }
  }
}

- (void)addResult:(id)result
{
  m_results.push_back(*[result get]);
  [m_table reloadData];
}


//****************************************************************** 
//*********** Location manager callbacks ***************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  // Handle location status changes if necessary
}

- (void)onGpsUpdate:(location::GpsInfo const &)info
{
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    [self updateCellDistance:cell withIndex:[m_table indexPathForCell:cell].row];
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    NSInteger const index = [m_table indexPathForCell:cell].row;
    if (m_results[index].GetResultType() == search::Result::RESULT_FEATURE)
      [self updateCellAngle:cell withIndex:index andAngle:((info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading)];
  }
}
//*********** End of Location manager callbacks ********************
//****************************************************************** 

//****************************************************************** 
//*********** Hack to keep Cancel button always enabled ************
- (void)enableCancelButton:(UISearchBar *)aSearchBar
{
  for (id subview in [aSearchBar subviews])
  {
    if ([subview isKindOfClass:[UIButton class]])
    {
      [subview setEnabled:TRUE];
      break;
    }
  }
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)aSearchBar
{
  [aSearchBar resignFirstResponder];
  [self performSelector:@selector(enableCancelButton:) withObject:aSearchBar afterDelay:0.0];
}
// ********** End of hack ******************************************
// *****************************************************************

@end
