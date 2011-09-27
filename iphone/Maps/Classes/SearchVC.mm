#import "SearchVC.h"
#import "CompassView.h"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"
#include "../../indexer/mercator.hpp"
#include "../../map/framework.hpp"
#include "../../search/result.hpp"

SearchVC * g_searchVC = nil;
SearchF g_searchF;
ShowRectF g_showRectF;
GetViewportCenterF g_getViewportCenterF;
volatile int g_queryId = 0;

@interface Wrapper : NSObject
{ // HACK: ownership is taken by the "get" method caller
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
    Wrapper * w = [[Wrapper alloc] initWithResult:res];
    [g_searchVC performSelectorOnMainThread:@selector(addResult:)
                                 withObject:w
                              waitUntilDone:NO];
    [w release];
  }
}

@implementation SearchVC

- (id)initWithSearchFunc:(SearchF)s andShowRectFunc:(ShowRectF)r 
  andGetViewportCenterFunc:(GetViewportCenterF)c
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    g_searchF = s;
    g_showRectF = r;
    g_getViewportCenterF = c;
    m_locationManager = [[CLLocationManager alloc] init];
    m_locationManager.delegate = self;
    // filter out unnecessary events
    m_locationManager.headingFilter = 3;
    m_locationManager.distanceFilter = 1.0;
    m_isRadarEnabled = false;
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

- (BOOL)locationManagerShouldDisplayHeadingCalibration:(CLLocationManager *)manager
{
  return YES;
}

- (void)locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
//  CLLocationDirection north = newHeading.trueHeading;
//  if (north < 0.)
//    north = newHeading.magneticHeading;
//  
//  UITableView * table = (UITableView *)self.view;
//  NSArray * indexes = [table indexPathsForVisibleRows];
//  for (NSUInteger i = 0; i < indexes.count; ++i)
//  {
//    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
//    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
//    CompassView * compass = (CompassView *)cell.accessoryView;
//    compass.north = north * M_PI / 180.;
//  }
}

- (void)onRadarButtonClick:(id)button
{
  m_isRadarEnabled = !m_isRadarEnabled;
  self.navigationItem.rightBarButtonItem.image = m_isRadarEnabled ? 
      [UIImage imageNamed:@"location-selected.png"] :
      [UIImage imageNamed:@"location.png"];
  if (m_isRadarEnabled)
  {
    [m_locationManager startUpdatingLocation];
    if ([CLLocationManager headingAvailable])
      [m_locationManager startUpdatingHeading];
  }
  else
  {
    if ([CLLocationManager headingAvailable])
      [m_locationManager stopUpdatingHeading];
    [m_locationManager stopUpdatingLocation];
  }
}

- (void)loadView
{
  UITableView * tableView = [[[UITableView alloc] init] autorelease];
  tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  tableView.delegate = self;
  tableView.dataSource = self;
  self.view = tableView;
  self.title = @"Search";

  UISearchBar * searchBar = [[[UISearchBar alloc] init] autorelease];
  searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.navigationItem.titleView = searchBar;
  [searchBar sizeToFit];
  searchBar.delegate = self;
  
  // add radar mode button
  UIImage * img = m_isRadarEnabled ? [UIImage imageNamed:@"location-selected.png"] : [UIImage imageNamed:@"location.png"];
  self.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithImage:img
                                                                            style:UIBarButtonItemStylePlain
                                                                           target:self
                                                                           action:@selector(onRadarButtonClick:)] autorelease];
}

- (void)viewDidLoad
{
  g_searchVC = self;
}

- (void)viewDidUnload
{
  [m_locationManager stopUpdatingHeading];
  [m_locationManager stopUpdatingLocation];
  g_searchVC = nil;
}

- (void)viewWillDisappear:(BOOL)animated
{
  // hide keyboard immediately
  [self.navigationItem.titleView resignFirstResponder];
}

- (void)searchBar:(UISearchBar *)sender textDidChange:(NSString *)searchText
{
  [self clearResults];
  [(UITableView *)self.view reloadData];
  ++g_queryId;

  // show active search indicator
  //  self.navigationItem.rightBarButtonItem.customView = [[[UIActivityIndicatorView alloc]
  //      initWithActivityIndicatorStyle: UIActivityIndicatorViewStyleGray] autorelease];

  if ([searchText length] > 0)
  {
    g_searchF([[searchText precomposedStringWithCompatibilityMapping] UTF8String],
              bind(&OnSearchResultCallback, _1, g_queryId));
  }
}

- (float)calculateAngle:(m2::PointD const &)pt
{
  if (m_isRadarEnabled)
  {
    CLLocation * loc = m_locationManager.location;
    if (loc)
      return ang::AngleTo(m2::PointD(MercatorBounds::LonToX(loc.coordinate.longitude),
                                   MercatorBounds::LatToY(loc.coordinate.latitude)), pt);
  }
  m2::PointD const center = g_getViewportCenterF();
  return ang::AngleTo(center, pt);
}

- (double)calculateDistance:(m2::PointD const &)pt
{
  double const ptLat = MercatorBounds::YToLat(pt.y);
  double const ptLon = MercatorBounds::XToLon(pt.x);
  if (m_isRadarEnabled)
  {
    CLLocation * loc = m_locationManager.location;
    if (loc)
      return ms::DistanceOnEarth(loc.coordinate.latitude, loc.coordinate.longitude,
                                          ptLat, ptLon);
  }
  m2::PointD const center = g_getViewportCenterF();
  return ms::DistanceOnEarth(MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x),
                             ptLat, ptLon);
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

    if (r.IsEndMarker())
    { // search is finished
      // hide search indicator
      //self.navigationItem.rightBarButtonItem.customView = nil;
    }
    else
    {
      cell.textLabel.text = [NSString stringWithUTF8String:r.GetString()];
      if (r.GetResultType() == search::Result::RESULT_FEATURE)
      {
        // @TODO use imperial system from the settings if needed
        // @TODO use meters too
        cell.detailTextLabel.text = [NSString stringWithFormat:@"%.1lf km", r.GetDistanceFromCenter() / 1000.0];

        float const h = tableView.rowHeight * 0.6;
        CompassView * v = [[[CompassView alloc] initWithFrame:
                          CGRectMake(0, 0, h, h)] autorelease];
        v.angle = [self calculateAngle:r.GetFeatureCenter()];
        cell.accessoryView = v;
      }
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
      g_showRectF(res.GetFeatureRect());
      [self.navigationController popViewControllerAnimated:YES];
      break;

    case search::Result::RESULT_SUGGESTION:
      [(UISearchBar *)self.navigationItem.titleView setText: [NSString stringWithFormat:@"%s ", res.GetSuggestionString()]];
      break;
    }
  }
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
}

- (void)addResult:(id)result
{
  m_results.push_back(*[result get]);
  [(UITableView *)self.view reloadData];
}

@end
