#import "SearchVC.h"
#import "CompassView.h"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"
#include "../../platform/settings.hpp"
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

/// Key to store settings
#define SEARCH_MODE_SETTING     "SearchMode"
#define SEARCH_MODE_POPULARITY  "ByPopularity"
#define SEARCH_MODE_ONTHESCREEN "OnTheScreen"
#define SEARCH_MODE_NEARME      "NearMe"
#define SEARCH_MODE_DEFAULT     SEARCH_MODE_POPULARITY

@implementation SearchVC

@synthesize m_searchBar;
@synthesize m_table;

- (void)setSearchMode:(string const &)mode
{
  if (mode == SEARCH_MODE_POPULARITY)
  {
    m_searchBar.selectedScopeButtonIndex = 0;
    // @TODO switch search mode
    //m_framework->SearchEngine()->SetXXXXXX();
  }
  else if (mode == SEARCH_MODE_ONTHESCREEN)
  {
    m_searchBar.selectedScopeButtonIndex = 1;
    // @TODO switch search mode
    //m_framework->SearchEngine()->SetXXXXXX();
  }
  else // Search mode "Near me"
  {
    m_searchBar.selectedScopeButtonIndex = 2;
    // @TODO switch search mode
    //m_framework->SearchEngine()->SetXXXXXX();
  }
  Settings::Set(SEARCH_MODE_SETTING, mode);
}

- (id)initWithFramework:(framework_t *)framework
{
  if ((self = [super initWithNibName:@"Search" bundle:nil]))
  {
    m_framework = framework;
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    // filter out unnecessary events
    m_locationManager.headingFilter = 3;
    m_locationManager.distanceFilter = 1.0;
    [m_locationManager startUpdatingLocation];
    if ([CLLocationManager headingAvailable])
      [m_locationManager startUpdatingHeading];
  
    // load previously saved search mode
    string searchMode;
    if (!Settings::Get(SEARCH_MODE_SETTING, searchMode))
      searchMode = SEARCH_MODE_DEFAULT;
    [self setSearchMode:searchMode];
  }
  
  return self;
}

- (void)clearResults
{
  m_results.clear();
}

- (void)dealloc
{
  [m_locationManager release];
  g_searchVC = nil;
  [self clearResults];
  [super dealloc];
}

- (void)viewDidLoad
{
  g_searchVC = self;
}

- (void)viewDidUnload
{
  if ([CLLocationManager headingAvailable])
    [m_locationManager stopUpdatingHeading];
  [m_locationManager stopUpdatingLocation];
  g_searchVC = nil;
  // to correctly free memory
  self.m_searchBar = nil;
  self.m_table = nil;
  m_results.clear();
  [super viewDidUnload];
}

- (void)fixHeadingOrientation
{
  m_locationManager.headingOrientation = (CLDeviceOrientation)[UIDevice currentDevice].orientation;
}

- (void)viewWillAppear:(BOOL)animated
{
  [self fixHeadingOrientation];
  // show keyboard
  [m_searchBar becomeFirstResponder];
  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  // hide keyboard immediately
  [m_searchBar resignFirstResponder];
  [super viewWillDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

// correctly pass rotation event up to the root mapViewController
// to fix rotation bug when other controller is above the root
- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
  [[self.navigationController.viewControllers objectAtIndex:0] didRotateFromInterfaceOrientation:fromInterfaceOrientation];
  [self fixHeadingOrientation];
}

//**************************************************************************
//*********** SearchBar handlers *******************************************
- (void)searchBar:(UISearchBar *)sender textDidChange:(NSString *)searchText
{
  [self clearResults];
  [m_table reloadData];
  ++g_queryId;

  if ([searchText length] > 0)
  {
    m_framework->Search([[searchText precomposedStringWithCompatibilityMapping] UTF8String],
              bind(&OnSearchResultCallback, _1, g_queryId));
  }
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  [self dismissModalViewControllerAnimated:YES];
}

- (void)searchBar:(UISearchBar *)searchBar selectedScopeButtonIndexDidChange:(NSInteger)selectedScope
{
  string searchMode;
  switch (selectedScope)
  {
  case 0: searchMode = SEARCH_MODE_POPULARITY; break;
  case 1: searchMode = SEARCH_MODE_ONTHESCREEN; break;
  default: searchMode = SEARCH_MODE_NEARME; break;
  }
  [self setSearchMode:searchMode];
}
//*********** End of SearchBar handlers *************************************
//***************************************************************************

- (double)calculateAngle:(m2::PointD const &)pt
{
  if ([CLLocationManager headingAvailable])
  {
    CLLocation * loc = m_locationManager.location;
    if (loc)
    {
      double angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
                                             MercatorBounds::LatToY(loc.coordinate.latitude)), pt);
      CLHeading * h = m_locationManager.heading;
      if (h)
      {
        CLLocationDirection northDeg = h.trueHeading;
        if (northDeg < 0.)
          northDeg = h.magneticHeading;
        double const northRad = northDeg / 180. * math::pi;
        angle += northRad;
      }
      return angle;
    }
  }
  
  m2::PointD const center = m_framework->GetViewportCenter();
  return ang::AngleTo(center, pt);
}

- (double)calculateDistance:(m2::PointD const &)pt
{
  double const ptLat = MercatorBounds::YToLat(pt.y);
  double const ptLon = MercatorBounds::XToLon(pt.x);
  CLLocation * loc = m_locationManager.location;
  if (loc)
    return ms::DistanceOnEarth(loc.coordinate.latitude, loc.coordinate.longitude, ptLat, ptLon);
  else
  {
    m2::PointD const center = m_framework->GetViewportCenter();
    return ms::DistanceOnEarth(MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x),
                             ptLat, ptLon);
  }
}

- (void)updateCellAngle:(UITableViewCell *)cell withIndex:(NSUInteger)index
{
  double const angle = [self calculateAngle:m_results[index].GetFeatureCenter()];  
  CompassView * compass = (CompassView *)cell.accessoryView;
  compass.angle = angle;
}

- (void)updateCellDistance:(UITableViewCell *)cell withIndex:(NSUInteger)index
{
  // @TODO use imperial system from the settings if needed
  // @TODO use meters too
  cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1lf km", 
                               [self calculateDistance:m_results[index].GetFeatureCenter()] / 1000.0];
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
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"MyTableViewCell"];
  if (!cell)
    cell = [[[UITableViewCell alloc]
           initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"MyTableViewCell"]
           autorelease];
  
  cell.accessoryView = nil;
  if (indexPath.row < m_results.size())
  {
    search::Result const & r = m_results[indexPath.row];

    cell.textLabel.text = [NSString stringWithUTF8String:r.GetString()];
    if (r.GetResultType() == search::Result::RESULT_FEATURE)
    {
      [self updateCellDistance:cell withIndex:indexPath.row];
      
      float const h = tableView.rowHeight * 0.6;
      CompassView * v = [[[CompassView alloc] initWithFrame:
                          CGRectMake(0, 0, h, h)] autorelease];
      cell.accessoryView = v;
      [self updateCellAngle:cell withIndex:indexPath.row];
    }
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
      [(UISearchBar *)self.navigationItem.titleView setText: [NSString stringWithFormat:@"%s ", res.GetSuggestionString()]];
      break;
    }
  }
}

- (void)addResult:(id)result
{
  m_results.push_back(*[result get]);
  [m_table reloadData];
}

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  return YES;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    [self updateCellAngle:cell withIndex:[m_table indexPathForCell:cell].row];
  }
}

- (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation
           fromLocation:(CLLocation *)oldLocation
{
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    [self updateCellDistance:cell withIndex:[m_table indexPathForCell:cell].row];
  }
}

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
