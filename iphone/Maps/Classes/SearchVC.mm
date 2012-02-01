#import "SearchVC.h"
#import "CompassView.h"
#import "LocationManager.h"
#import "SearchBannerChecker.h"
#import "SearchCell.h"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"
#include "../../platform/settings.hpp"
#include "../../platform/platform.hpp"
#include "../../indexer/mercator.hpp"
#include "../../map/framework.hpp"
#include "../../map/measurement_utils.hpp"
#include "../../search/result.hpp"

SearchVC * g_searchVC = nil;
volatile int g_queryId = 0;

@interface Wrapper : NSObject
{
  search::Results * m_result;
}
- (id)initWithResult:(search::Results const &) res;
- (search::Results const *)get;
@end

@implementation Wrapper
- (id)initWithResult:(search::Results const &) res
{
  if ((self = [super init]))
    m_result = new search::Results(res);
  return self;
}

- (void)dealloc
{
  delete m_result;
  [super dealloc];
}

- (search::Results const *)get
{
  return m_result;
}
@end

static void OnSearchResultCallback(search::Results const & res, int queryId)
{
  if (g_searchVC && queryId == g_queryId)
  {
    Wrapper * w = [[Wrapper alloc] initWithResult:res];
    [g_searchVC performSelectorOnMainThread:@selector(addResult:)
                                 withObject:w
                              waitUntilDone:NO];
    [w release];
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

  m_framework->GetSearchEngine()->EnablePositionTrack(true);
}

- (void)disableRadarMode
{
  m_radarButton.selected = NO;

  m_framework->GetSearchEngine()->EnablePositionTrack(false);
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

+ (NSString *)formatDistance:(double)meters
{
  if (meters < 0.)
    return nil;

  uint64_t shortUnits = (uint64_t)meters;
  double longUnits = meters/1000.0;
  // @TODO localize measurements
  static NSString * shortLabel = @"m";
  static NSString * longLabel = @"km";
  Settings::Units u = Settings::Metric;
	Settings::Get("Units", u);
  switch (u)
  {
  case Settings::Foot:
    shortUnits = (uint64_t)MeasurementUtils::MetersToFeet(meters);
    longUnits = MeasurementUtils::MetersToMiles(meters);
    shortLabel = @"ft";
    longLabel = @"mi";
    break;
    
  case Settings::Yard:
    shortUnits = (uint64_t)MeasurementUtils::MetersToYards(meters);
    longUnits = MeasurementUtils::MetersToMiles(meters);
    shortLabel = @"yd";
    longLabel = @"mi";
    break;

  case Settings::Metric:
    shortLabel = @"m";
    longLabel = @"km";
    break;
  }

  // NSLocalizedString(@"%.1lf m", @"Search results - Metres")
  // NSLocalizedString(@"%.1lf ft", @"Search results - Feet")
  // NSLocalizedString(@"%.1lf mi", @"Search results - Miles")
  // NSLocalizedString(@"%.1lf yd", @"Search results - Yards")

  if (shortUnits < 1000)
    return [NSString stringWithFormat:@"%qu %@", shortUnits, shortLabel];

  uint64_t const longUnitsRounded = (uint64_t)(longUnits);
  // reduce precision for big distances and remove zero for x.0-like numbers
  if (longUnitsRounded > 10 || (longUnitsRounded && (uint64_t)(longUnits * 10.0) == longUnitsRounded * 10))
    return [NSString stringWithFormat:@"%qu %@", longUnitsRounded, longLabel];

  return [NSString stringWithFormat:@"%.1lf %@", longUnits, longLabel];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_results.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row >= m_results.size())
  {
    ASSERT(false, ("Invalid m_results with size", m_results.size()));
    return nil;
  }

  search::Result const & r = m_results[indexPath.row];
  switch (r.GetResultType())
  {
    case search::Result::RESULT_FEATURE:
    {
      SearchCell * cell = (SearchCell *)[tableView dequeueReusableCellWithIdentifier:@"FeatureCell"];
      if (!cell)
        cell = [[[SearchCell alloc] initWithReuseIdentifier:@"FeatureCell"] autorelease];

      cell.featureName.text = [NSString stringWithUTF8String:r.GetString()];
      cell.featureCountry.text = [NSString stringWithUTF8String:r.GetRegionString()];
      cell.featureType.text = [NSString stringWithUTF8String:r.GetFeatureType()];
      cell.featureDistance.text = [SearchVC formatDistance:r.GetDistanceFromCenter()];
      return cell;
    }
    break;

    case search::Result::RESULT_SUGGESTION:
    {
      UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"SuggestionCell"];
      if (!cell)
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"SuggestionCell"] autorelease];
      cell.textLabel.text = [NSString stringWithUTF8String:r.GetString()];
      return cell;
    }
    break;

    default:
      ASSERT(false, ("Unsupported search result type"));
    return nil;
  }
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
  m_results.clear();

  search::Results const * r = [result get];
  m_results.insert(m_results.end(), r->Begin(), r->End());
  [m_table reloadData];
}


- (void)updateCompassFor:(UITableViewCell *)cell withResult:(search::Result const &)res andAngle:(double)angle
{
  CompassView * compass = nil;
  if (cell.accessoryView == nil)
  {
    float const h = m_table.rowHeight * 0.6;
    compass = [[CompassView alloc] initWithFrame:CGRectMake(0, 0, h, h)];
    cell.accessoryView = compass;
    [compass release];
  }
  else if ([cell.accessoryView isKindOfClass:[CompassView class]])
    compass = (CompassView *)cell.accessoryView;

  if (compass)
    compass.angle = angle;
}

//****************************************************************** 
//*********** Location manager callbacks ***************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  // Handle location status changes if necessary
}

- (void)onGpsUpdate:(location::GpsInfo const &)info
{
  m_framework->GetSearchEngine()->SetPosition(info.m_latitude, info.m_longitude);
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  CLLocation * loc = m_locationManager.lastLocation;
  if (loc == nil)
    return;

  double const northDeg = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    search::Result const & res = m_results[[m_table indexPathForCell:cell].row];
    if (res.GetResultType() == search::Result::RESULT_FEATURE)
    {
      m2::PointD const center = res.GetFeatureCenter();
      double const angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
          MercatorBounds::LatToY(loc.coordinate.latitude)), center) + northDeg / 180. * math::pi;
      [self updateCompassFor:cell withResult:res andAngle:angle];
    }
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

// Dismiss virtual keyboard when touching tableview
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
  [m_searchBar resignFirstResponder];
}

// Dismiss virtual keyboard when "Search" button is pressed
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
  [m_searchBar resignFirstResponder];
}

@end
