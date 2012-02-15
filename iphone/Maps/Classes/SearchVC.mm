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


/// When to display compass instead of country flags
#define MIN_COMPASS_DISTANCE 25000.0

SearchVC * g_searchVC = nil;
volatile int g_queryId = 0;

@interface ResultsWrapper : NSObject
{
  vector<search::Result> m_results;
  NSString * m_searchString;
  int m_queryId;
}
// Stores search string which corresponds to these results.
@property(nonatomic,retain) NSString * m_searchString;
// Used to double-checking and discarding old results in GUI thread
@property(nonatomic,assign) int m_queryId;
- (id)initWithResults:(search::Results const &) res andQueryId:(int)qId;
- (vector<search::Result> const &)get;
@end

@implementation ResultsWrapper

@synthesize m_searchString;
@synthesize m_queryId;

- (id)initWithResults:(search::Results const &)res andQueryId:(int)qId
{
  if ((self = [super init]))
  {
    m_results.assign(res.Begin(), res.End());
    m_queryId = qId;
  }
  return self;
}

- (vector<search::Result> const &)get
{
  return m_results;
}
@end

// Last search results are stored betweel SearchVC sessions
// to appear instantly for the user, they also store last search text query
ResultsWrapper * g_lastSearchResults = nil;

static void OnSearchResultCallback(search::Results const & res, int queryId)
{
  int currQueryId = g_queryId;
  if (g_searchVC && queryId == currQueryId)
  {
    ResultsWrapper * w = [[ResultsWrapper alloc] initWithResults:res andQueryId:currQueryId];
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
  navBar.topItem.leftBarButtonItem.customView.bounds = CGRectMake(0, 0, wAndH, wAndH);

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


+ (BOOL)isLocationValid:(CLLocation *)l
{
  if (l == nil) return false;
  
  // do not use too old locations
  static NSTimeInterval const SECONDS_TO_EXPIRE = 300.0;
  
  // timeIntervalSinceNow returns negative value - because of "since now"
  return [l.timestamp timeIntervalSinceNow] > (-SECONDS_TO_EXPIRE);
}

- (id)initWithFramework:(Framework *)framework andLocationManager:(LocationManager *)lm
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    m_framework = framework;
    m_locationManager = lm;
    
    // get current location
    CLLocation * l = m_locationManager.lastLocation;
    
    double lat, lon;
    bool const hasPt = [SearchVC isLocationValid:l];
    if (hasPt)
    {
      lat = l.coordinate.latitude;
      lon = l.coordinate.longitude;
    }
    
    m_framework->PrepareSearch(hasPt, lat, lon);
  }
  return self;
}

- (void)enableRadarMode
{
  m_radarButton.selected = YES;
}

- (void)disableRadarMode
{
  m_radarButton.selected = NO;
}

- (void)fillSearchParams:(search::SearchParams &)params withText:(NSString *)queryString
{
  params.m_query = [[queryString precomposedStringWithCompatibilityMapping] UTF8String];
  params.m_callback = bind(&OnSearchResultCallback, _1, g_queryId);
  // Set current keyboard input mode
  params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);
  
  bool radarEnabled = m_radarButton.selected == YES;
  CLLocation * l = m_locationManager.lastLocation;
  
  if ([SearchVC isLocationValid:l])
    params.SetPosition(l.coordinate.latitude, l.coordinate.longitude);
  else
    radarEnabled = false;
  
  params.SetNearMeMode(radarEnabled);
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

  // Refresh search results with new mode, but
  // do not search if text is not entered
  NSString * queryString = m_searchBar.text;
  if (queryString.length)
  {
    search::SearchParams params;
    [self fillSearchParams:params withText:queryString];
    m_framework->Search(params);
  }
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
  UIBarButtonItem * closeButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Done", @"Search Results - Close search button") style: UIBarButtonItemStyleDone
                                                                   target:self action:@selector(onCloseButton:)] autorelease];
  item.rightBarButtonItem = closeButton;


  m_searchBar = [[UISearchBar alloc] init];
  m_searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  // restore previous search query
  if (g_lastSearchResults)
    m_searchBar.text = g_lastSearchResults.m_searchString;
  m_searchBar.delegate = self;
  m_searchBar.placeholder = NSLocalizedString(@"Search map", @"Search box placeholder text");
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

- (void)dealloc
{
  g_searchVC = nil;
  [m_radarButton release];
  [m_searchBar release];
  [m_table release];
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
  ++g_queryId;

  if (searchText.length)
  {
    search::SearchParams params;
    [self fillSearchParams:params withText:searchText];
    m_framework->Search(params);
  }
  else
  {
    [g_lastSearchResults release];
    g_lastSearchResults = nil;
    // Clean the table
    [m_table reloadData];
  }
}

- (void)onCloseButton:(id)sender
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
  if (g_lastSearchResults)
    return [g_lastSearchResults get].size();
  else
    return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (g_lastSearchResults == nil || indexPath.row >= [g_lastSearchResults get].size())
  {
    ASSERT(false, ("Invalid m_results with size", [g_lastSearchResults get].size()));
    return nil;
  }

  search::Result const & r = [g_lastSearchResults get][indexPath.row];
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
      double const distance = r.GetDistanceFromCenter();
      cell.featureDistance.text = [SearchVC formatDistance:distance];
      // Show flags only if feature is too far away and it has the flag
      if (r.GetRegionFlag() && (distance < 0. || distance > MIN_COMPASS_DISTANCE))
      {
        UIImage * flagImage = [UIImage imageNamed:[NSString stringWithFormat:@"%s.png", r.GetRegionFlag()]];
        UIImageView * imgView = [[UIImageView alloc] initWithImage:flagImage];
        cell.accessoryView = imgView;
        [imgView release];
      }
      else
      {
        CompassView * compass;
        // Try to reuse existing compass view
        if ([cell.accessoryView isKindOfClass:[CompassView class]])
          compass = (CompassView *)cell.accessoryView;
        else
        {
          // Create compass view
          float const h = (int)(m_table.rowHeight * 0.6);
          compass = [[CompassView alloc] initWithFrame:CGRectMake(0, 0, h, h)];
          cell.accessoryView = compass;
          [compass release];
        }

        // Separate case for continents
        if (!r.GetRegionFlag())
        {
          // @TODO add valid icon
          compass.image = [UIImage imageNamed:@"downloader"];
        }
        else
        {
          CLLocation * loc = [m_locationManager lastLocation];
          CLHeading * heading = [m_locationManager lastHeading];
          if (loc == nil || heading == nil)
          {
            compass.image = [UIImage imageNamed:@"location"];
          }
          else
          {
            double const northDeg = (heading.trueHeading < 0) ? heading.magneticHeading : heading.trueHeading;
            m2::PointD const center = r.GetFeatureCenter();
            compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
                            MercatorBounds::LatToY(loc.coordinate.latitude)), center) + northDeg / 180. * math::pi;
          }
        }
      }
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
  if (indexPath.row < [g_lastSearchResults get].size())
  {
    search::Result const & res = [g_lastSearchResults get][indexPath.row];
    switch(res.GetResultType())
    {
      // Zoom to the feature
    case search::Result::RESULT_FEATURE:
      m_framework->ShowRect(res.GetFeatureRect());
      [self onCloseButton:nil];
      break;

    case search::Result::RESULT_SUGGESTION:
      char const * s = res.GetSuggestionString();
      [m_searchBar setText: [NSString stringWithUTF8String:s]];
      // Remove blue selection from the row
      [tableView deselectRowAtIndexPath: indexPath animated:YES];
      break;
    }
  }
}

// Called on UI thread from search threads
- (void)addResult:(id)res
{
  ResultsWrapper * w = (ResultsWrapper *)res;
  // Additional check to discard old results when user entered new text query
  if (g_queryId == w.m_queryId)
  {
    [g_lastSearchResults release];
    g_lastSearchResults = [w retain];
    w.m_searchString = m_searchBar.text;
    [m_table reloadData];
  }
}

//****************************************************************** 
//*********** Location manager callbacks ***************************
- (void)onLocationStatusChanged:(location::TLocationStatus)newStatus
{
  // Handle location status changes if necessary
}

- (void)onGpsUpdate:(location::GpsInfo const &)info
{
  // Refresh search results with newer location, but
  // do not search if text is not entered
  NSString * queryString = m_searchBar.text;
  if (queryString.length)
  {
    search::SearchParams params;
    [self fillSearchParams:params withText:queryString];
    m_framework->Search(params);
  }
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
    search::Result const & res = [g_lastSearchResults get][[m_table indexPathForCell:cell].row];
    if (res.GetResultType() == search::Result::RESULT_FEATURE)
    {
      // Show compass only for cells without flags
      if ([cell.accessoryView isKindOfClass:[CompassView class]])
      {
        CompassView * compass = (CompassView *)cell.accessoryView;
        m2::PointD const center = res.GetFeatureCenter();
        compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
                        MercatorBounds::LatToY(loc.coordinate.latitude)), center) + northDeg / 180. * math::pi;
      }
    }
  }
}
//*********** End of Location manager callbacks ********************
//****************************************************************** 

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
