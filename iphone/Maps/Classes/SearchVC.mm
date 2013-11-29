#import "SearchVC.h"
#import "CompassView.h"
#import "LocationManager.h"
#import "SearchCell.h"
#import "BookmarksVC.h"
#import "CustomNavigationView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "Statistics.h"
#import "Config.h"
#import "UIKitCategories.h"

#include "Framework.h"

#include "../../search/result.hpp"
#include "../../search/params.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"
#include "../../platform/settings.hpp"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"

/// When to display compass instead of country flags
#define MIN_COMPASS_DISTANCE 25000.0

/// To save search scope selection between launches
#define SEARCH_MODE_SETTING "SearchMode"

SearchVC * g_searchVC = nil;

@interface ResultsWrapper : NSObject
{
  search::Results m_results;
}

// Stores search string which corresponds to these results.
@property(nonatomic, retain) NSString * searchString;

- (id)initWithResults:(search::Results const &)res;

- (int)getCount;
- (search::Result const &)getWithPosition:(NSInteger)pos;

- (BOOL)isEndMarker;
- (BOOL)isEndedNormal;

@end

@implementation ResultsWrapper

@synthesize searchString;

- (id)initWithResults:(search::Results const &)res
{
  if ((self = [super init]))
    m_results = res;
  return self;
}

- (int)getCount
{
  return static_cast<int>(m_results.GetCount());
}

- (search::Result const &)getWithPosition:(NSInteger)pos
{
  return m_results.GetResult(pos);
}

- (BOOL)isEndMarker
{
  return m_results.IsEndMarker();
}

- (BOOL)isEndedNormal
{
  return m_results.IsEndedNormal();
}

@end


// Last search results are stored between SearchVC sessions
// to appear instantly for the user, they also store last search text query.
//ResultsWrapper * g_lastSearchResults = nil;

static NSInteger GetDefaultSearchScope()
{
  NSInteger value;
  if (Settings::Get(SEARCH_MODE_SETTING, value))
    return value;
  return 0; // 0 is default scope ("Near me")
}

NSString * g_lastSearchRequest = nil;
NSInteger g_scopeSection = GetDefaultSearchScope();
NSInteger g_numberOfRowsInEmptySearch = 0;

static void OnSearchResultCallback(search::Results const & res)
{
  if (g_searchVC)
  {
    ResultsWrapper * w = [[ResultsWrapper alloc] initWithResults:res];
    [g_searchVC performSelectorOnMainThread:@selector(addResult:)
                                 withObject:w waitUntilDone:NO];
  }
}

/////////////////////////////////////////////////////////////////////

@interface SearchVC ()

@property (nonatomic) BOOL searching;
@property (nonatomic) UIActivityIndicatorView *activityIndicator;

@end

@implementation SearchVC
@synthesize searchResults = _searchResults;

- (id) init
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    m_framework = &GetFramework();
    m_locationManager = [MapsAppDelegate theApp].m_locationManager;

    double lat, lon;
    bool const hasPt = [m_locationManager getLat:lat Lon:lon];
    m_framework->PrepareSearch(hasPt, lat, lon);

    //mycode init array of categories
    categoriesNames = [[NSArray alloc] initWithObjects:
                       @"food",
                       @"shop",
                       @"hotel",
                       @"tourism",
                       @"entertainment",
                       @"atm",
                       @"bank",
                       @"transport",
                       @"fuel",
                       @"parking",
                       @"pharmacy",
                       @"hospital",
                       @"toilet",
                       @"post",
                       @"police",
                       nil];
    _searchResults = [[NSMutableArray alloc] initWithObjects:[[ResultsWrapper alloc] init], [[ResultsWrapper alloc] init], [[ResultsWrapper alloc] init], nil];
    if (!g_lastSearchRequest)
    {
      g_lastSearchRequest = @"";
    }
  }
  return self;
}

- (void)fillSearchParams:(search::SearchParams &)params withText:(NSString *)queryString
{
  params.m_query = [[queryString precomposedStringWithCompatibilityMapping] UTF8String];
  params.m_callback = bind(&OnSearchResultCallback, _1);

  // Set current keyboard input mode
  // Note: input mode was introduced in iOS 4.2 (now we support 4.3+)
  params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);

  double lat, lon;
  if ([m_locationManager getLat:lat Lon:lon])
    params.SetPosition(lat, lon);
}



- (UISearchBar *)searchBar
{
  if (!_searchBar)
  {
    _searchBar = [[UISearchBar alloc] initWithFrame:CGRectMake(0, 0, self.view.width - 80, 44)];
    _searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _searchBar.delegate = self;
    _searchBar.placeholder = NSLocalizedString(@"search_map", @"Search box placeholder text");
    _searchBar.barStyle = UISearchBarStyleDefault;
    _searchBar.tintColor = [[UINavigationBar appearance] tintColor];

    if (g_lastSearchRequest)
      [_searchBar setText:g_lastSearchRequest];
  }
  return _searchBar;
}

- (ScopeView *)scopeView
{
  if (!_scopeView)
  {
    UISegmentedControl * segmentedControl = [[UISegmentedControl alloc] initWithItems:@[NSLocalizedString(@"search_mode_nearme", nil),
                                                                                        NSLocalizedString(@"search_mode_viewport", nil),
                                                                                        NSLocalizedString(@"search_mode_all", nil)]];
    CGFloat width = IPAD ? 400 : 310;
    segmentedControl.frame = CGRectMake(0, 0, width, 32);
    if (SYSTEM_VERSION_IS_LESS_THAN(@"7"))
      segmentedControl.tintColor = [[UINavigationBar appearance] tintColor];
    else
      segmentedControl.tintColor = [UIColor whiteColor];

    segmentedControl.selectedSegmentIndex = GetDefaultSearchScope();
    segmentedControl.segmentedControlStyle = UISegmentedControlStyleBar;
    [segmentedControl addTarget:self action:@selector(segmentedControlValueChanged:) forControlEvents:UIControlEventValueChanged];
    _scopeView = [[ScopeView alloc] initWithFrame:CGRectMake(0, 0, self.view.width, 44) segmentedControl:segmentedControl];
    _scopeView.autoresizingMask = UIViewAutoresizingFlexibleWidth;

    if (SYSTEM_VERSION_IS_LESS_THAN(@"7"))
      _scopeView.backgroundColor = [[UINavigationBar appearance] tintColor];
    else
      _scopeView.backgroundColor = [UIColor colorWithColorCode:@"303e57"];
  }
  return _scopeView;
}

- (void)segmentedControlValueChanged:(UISegmentedControl *)segmentedControl
{
  g_scopeSection = segmentedControl.selectedSegmentIndex;
  // Save selected search mode for future launches
  Settings::Set(SEARCH_MODE_SETTING, g_scopeSection);
  [self proceedSearchWithString:self.searchBar.text andForceSearch:YES];
}

- (void)viewDidLoad
{
  g_searchVC = self;

  m_table = [[UITableView alloc] initWithFrame:self.view.bounds];
  m_table.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  m_table.delegate = self;
  m_table.dataSource = self;
  m_table.contentInset = UIEdgeInsetsMake(self.scopeView.height, 0, 0, 0);
  m_table.scrollIndicatorInsets = m_table.contentInset;

  self.view.backgroundColor = [UIColor whiteColor];
  [self.view addSubview:m_table];
  [self.view addSubview:self.scopeView];

  self.navigationItem.titleView = self.searchBar;
}

//// Banner dialog handler
//- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
//{
//  if (buttonIndex != alertView.cancelButtonIndex)
//  {
//    // Launch appstore
//    [APP openURL:[NSURL URLWithString:MAPSWITHME_PREMIUM_APPSTORE_URL]];
//    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"YES"];
//  }
//  else
//    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"NO"];
//
//  // Close view
//  [self.navigationController popToRootViewControllerAnimated:YES];
//}

- (void)viewWillAppear:(BOOL)animated
{
//  // Disable search for free version
//  if (!GetPlatform().IsPro())
//  {
//    // Display banner for paid version
//    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"search_available_in_pro_version", nil)
//                                                     message:nil
//                                                    delegate:self
//                                           cancelButtonTitle:NSLocalizedString(@"cancel", nil)
//                                           otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
//
//    [alert show];
//  }
//  else
//  {
  [m_locationManager start:self];
    // show keyboard
  [self.searchBar becomeFirstResponder];
//  }
  [super viewWillAppear:animated];

  [self performAfterDelay:0 block:^{
    [self resizeNavigationBar];
  }];
}

- (void)resizeNavigationBar
{
  CGFloat landscapeHeight = 32;
  CGFloat portraitHeight = 44;
  self.navigationController.navigationBar.height = portraitHeight;
  if (UIDeviceOrientationIsLandscape(self.interfaceOrientation))
    m_table.minY = portraitHeight - landscapeHeight;
  else
    m_table.minY = 0;
  self.scopeView.minY = m_table.minY;
}

- (void)viewDidAppear:(BOOL)animated
{
  // Relaunch search when view has appeared because of search indicator hack
  // (we replace one control with another, and system calls unsupported method on it)
  if (GetPlatform().IsPro())
    [self proceedSearchWithString:g_lastSearchRequest andForceSearch:YES];
}


- (void)viewWillDisappear:(BOOL)animated
{
  [m_locationManager stop:self];

  // hide keyboard immediately
  [self.searchBar resignFirstResponder];
  [super viewWillDisappear:animated];
  g_numberOfRowsInEmptySearch = 0;
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)orientation  duration:(NSTimeInterval)duration
{
  [self resizeNavigationBar];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;  // All orientations are supported.
}

//**************************************************************************
//*********** SearchBar handlers *******************************************
- (void)searchBar:(UISearchBar *)sender textDidChange:(NSString *)searchText
{
  self.searching = [searchText length] > 0;
  g_lastSearchRequest = [[NSString alloc] initWithString:self.searchBar.text];
  [self clearCacheResults];
  [self proceedSearchWithString:self.searchBar.text andForceSearch:YES];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar
{
  [searchBar resignFirstResponder];
}

- (void)onCloseButton:(id)sender
{
  [self.navigationController popToRootViewControllerAnimated:YES];
}
//*********** End of SearchBar handlers *************************************
//***************************************************************************

- (void)setSearchBoxText:(NSString *)text
{
  self.searchBar.text = text;
  // Manually send text change notification if control has no focus,
  // OR if iOS 6 - it doesn't send textDidChange notification after text property update
  if (![self.searchBar isFirstResponder]
      || [UIDevice currentDevice].systemVersion.floatValue > 5.999)
    [self searchBar:self.searchBar textDidChange:text];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  m_suggestionsCount = self.searchBar.text.length ? 0 : 1;
  //No text in search show categories
  if (m_suggestionsCount)
  {
    return [categoriesNames count];
  }
  //If no results we should show 0 strings if search is in progress or 1 string with message thaht there is no results
  if (![[_searchResults objectAtIndex:g_scopeSection] getCount])
  {
    return g_numberOfRowsInEmptySearch;
  }
  return [[_searchResults objectAtIndex:g_scopeSection] getCount];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSInteger realRowIndex = indexPath.row;
  if (m_suggestionsCount)
  {
    static NSString *CellIdentifier = @"categoryCell";

    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
    }

    cell.textLabel.text =  NSLocalizedString([categoriesNames objectAtIndex:indexPath.row], nil);
    cell.imageView.image = [UIImage imageNamed:[categoriesNames objectAtIndex:indexPath.row]];

    return cell;
  }
  //No search results
  if ([self.searchBar.text length] != 0 && ![[_searchResults objectAtIndex:g_scopeSection] getCount] && g_numberOfRowsInEmptySearch)
  {
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"NoResultsCell"];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"NoResultsCell"];
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
    }
    cell.textLabel.text = NSLocalizedString(@"no_search_results_found", nil);

    // check if we have no position in "near me" screen
    if (![m_locationManager lastLocationIsValid] && g_scopeSection == 0)
      cell.detailTextLabel.text = NSLocalizedString(@"unknown_current_position", nil);
    else
      cell.detailTextLabel.text = @"";
    return cell;
  }

  if ([_searchResults objectAtIndex:g_scopeSection] == nil || realRowIndex >= (NSInteger)[[_searchResults objectAtIndex:g_scopeSection] getCount])
  {
    ASSERT(false, ("Invalid m_results with size", [[_searchResults objectAtIndex:g_scopeSection] getCount]));
    return nil;
  }

  search::Result const & r = [[_searchResults objectAtIndex:g_scopeSection] getWithPosition:realRowIndex];
  if (r.GetResultType() != search::Result::RESULT_SUGGESTION)
  {
    SearchCell * cell = (SearchCell *)[tableView dequeueReusableCellWithIdentifier:@"FeatureCell"];
    if (!cell)
      cell = [[SearchCell alloc] initWithReuseIdentifier:@"FeatureCell"];

    // Init common parameters
    cell.featureName.text = [NSString stringWithUTF8String:r.GetString()];
    cell.featureCountry.text = [NSString stringWithUTF8String:r.GetRegionString()];
    cell.featureType.text = [NSString stringWithUTF8String:r.GetFeatureType()];

    // Get current position and compass "north" direction
    double azimut = -1.0;
    double lat, lon;

    if ([m_locationManager getLat:lat Lon:lon])
    {
      double north = -1.0;
      [m_locationManager getNorthRad:north];

      string distance;
      if (!m_framework->GetDistanceAndAzimut(r.GetFeatureCenter(),
                                             lat, lon, north, distance, azimut))
      {
        // do not draw arrow for far away features
        azimut = -1.0;
      }

      cell.featureDistance.text = [NSString stringWithUTF8String:distance.c_str()];
    }
    else
      cell.featureDistance.text = nil;

    // Show flags only if it has one and no azimut to feature
    char const * flag = r.GetRegionFlag();
    if (flag && azimut < 0.0)
    {
      UIImage * flagImage = [UIImage imageNamed:[NSString stringWithFormat:@"%s.png", flag]];
      UIImageView * imgView = [[UIImageView alloc] initWithImage:flagImage];
      cell.accessoryView = imgView;
    }
    else
    {
      // Try to reuse existing compass view
      CompassView * compass;
      if ([cell.accessoryView isKindOfClass:[CompassView class]])
        compass = (CompassView *)cell.accessoryView;
      else
      {
        // Create compass view
        float const h = (int)(m_table.rowHeight * 0.6);
        compass = [[CompassView alloc] initWithFrame:CGRectMake(0, 0, h, h)];
        cell.accessoryView = compass;
      }

      // Show arrow for valid azimut and if feature is not a continent (flag is exist)
      compass.showArrow = (azimut >= 0.0 && flag) ? YES : NO;
      compass.angle = azimut;
    }
    return cell;
  }
  else
  {
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"SuggestionCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"SuggestionCell"];
    cell.textLabel.text = [NSString stringWithUTF8String:r.GetString()];
    return cell;
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSInteger realRowIndex = indexPath.row;
  // Suggestion cell was clicked
  if (m_suggestionsCount)
  {
    [[Statistics instance] logEvent:@"Category Selection" withParameters:@{@"Category" : [categoriesNames objectAtIndex:realRowIndex]}];
    [self setSearchBoxText:[NSLocalizedString([categoriesNames objectAtIndex:realRowIndex], Search Suggestion) stringByAppendingString:@" "]];
    [m_table scrollRectToVisible:CGRectMake(0, 0, 1, 1) animated:YES];
    return;
  }

  if (realRowIndex < (NSInteger)[[_searchResults objectAtIndex:g_scopeSection] getCount])
  {
    search::Result const & res = [[_searchResults objectAtIndex:g_scopeSection] getWithPosition:realRowIndex];
    if (res.GetResultType() != search::Result::RESULT_SUGGESTION)
    {
      m_framework->ShowSearchResult(res);

      search::AddressInfo info;
      info.MakeFrom(res);

      if (g_scopeSection == 0)
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"Near Me"}];
      else if (g_scopeSection == 1)
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"On the Screen"}];
      else
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"Everywhere"}];

      [[MapsAppDelegate theApp].m_mapViewController showSearchResultAsBookmarkAtMercatorPoint:res.GetFeatureCenter() withInfo:info];

      [self onCloseButton:nil];
    }
    else
    {
      [self setSearchBoxText:[NSString stringWithUTF8String:res.GetSuggestionString()]];
      // Remove blue selection from the row
      [tableView deselectRowAtIndexPath: indexPath animated:YES];
    }
  }
}

- (void)setSearching:(BOOL)searching
{
  if (searching)
    [self.activityIndicator startAnimating];
  else
    [self.activityIndicator stopAnimating];

  [UIView animateWithDuration:0.15 animations:^{
    self.activityIndicator.alpha = searching ? 1 : 0;
    m_table.alpha = searching ? 0 : 1;
  }];

  _searching = searching;
}

- (UIActivityIndicatorView *)activityIndicator
{
  if (!_activityIndicator)
  {
    _activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    CGFloat activityX = self.view.width / 2;
    CGFloat activityY = self.scopeView.maxY + ((self.view.height - KEYBOARD_HEIGHT) - self.scopeView.maxY) / 2;
    _activityIndicator.center = CGPointMake(activityX, activityY);
    _activityIndicator.alpha = 0;
    [self.view addSubview:_activityIndicator];
  }
  return _activityIndicator;
}

// Called on UI thread from search threads
- (void)addResult:(id)res
{
  ResultsWrapper * w = (ResultsWrapper *)res;

  if ([w isEndMarker])
  {
    if ([w isEndedNormal])
    {
      g_numberOfRowsInEmptySearch = 1;
      self.searching = NO;
      [m_table reloadData];
    }
  }
  else
  {
    g_numberOfRowsInEmptySearch = 0;
    [_searchResults replaceObjectAtIndex:g_scopeSection withObject:w];
    [m_table reloadData];
  }
}

void setSearchType(search::SearchParams & params)
{
  switch (g_scopeSection)
  {
    case 0:
      params.SetSearchMode(search::SearchParams::AROUND_POSITION);
      break;
    case 1:
      params.SetSearchMode(search::SearchParams::IN_VIEWPORT);
      break;
    case 2:
      params.SetSearchMode(search::SearchParams::ALL);
      break;
    default:
      params.SetSearchMode(search::SearchParams::ALL);
      break;
  }
}

//******************************************************************
//*********** Location manager callbacks ***************************
- (void)onLocationError:(location::TLocationError)errorCode
{
  // Handle location status changes if necessary
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // Refresh search results with newer location.
  if (![self.searchBar.text length])
    return;
  search::SearchParams params;
  setSearchType(params);
  if (self.searchBar.text)
  {
    [self fillSearchParams:params withText:self.searchBar.text];

    // hack, fillSearch Params return invalid position
    params.SetPosition(info.m_latitude, info.m_longitude);

    g_numberOfRowsInEmptySearch = m_framework->Search(params) ? 0 : 1;
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  double lat, lon;
  if (![m_locationManager getLat:lat Lon:lon])
    return;
  //check if categories are on the screen
  if (!m_suggestionsCount && [[_searchResults objectAtIndex:g_scopeSection] getCount])
  {
    double const northRad = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
    NSArray * cells = m_table.visibleCells;
    for (NSUInteger i = 0; i < cells.count; ++i)
    {
      UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
      NSInteger realRowIndex = [m_table indexPathForCell:cell].row;
      search::Result const & res = [[_searchResults objectAtIndex:g_scopeSection] getWithPosition:realRowIndex];
      if (res.GetResultType() != search::Result::RESULT_SUGGESTION)
      {
        // Show compass only for cells without flags
        if ([cell.accessoryView isKindOfClass:[CompassView class]])
        {
          CompassView * compass = (CompassView *)cell.accessoryView;
          m2::PointD const center = res.GetFeatureCenter();
          compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                                  MercatorBounds::LatToY(lat)), center) + northRad;
        }
      }
    }
  }
}
//*********** End of Location manager callbacks ********************
//******************************************************************

// Dismiss virtual keyboard when touching tableview
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
  [self.searchBar resignFirstResponder];
}

// Dismiss virtual keyboard when "Search" button is pressed
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
  [self.searchBar resignFirstResponder];
}

// Callback from suggestion cell, called when icon is selected
- (void)onSuggestionSelected:(NSString *)suggestion
{
  [self setSearchBoxText:[suggestion stringByAppendingString:@" "]];
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  [self onCloseButton:nil];
}

-(void)clearCacheResults
{
  for (int i = 0; i < [_searchResults count]; ++i)
  {
    [_searchResults replaceObjectAtIndex:i withObject:[[ResultsWrapper alloc] init]];
  }
}

-(void)proceedSearchWithString:(NSString *)searchText andForceSearch:(BOOL)forceSearch
{
  g_numberOfRowsInEmptySearch = 0;
  [m_table reloadData];
  if (![searchText length])
    return;
  search::SearchParams params;
  setSearchType(params);
  if(forceSearch)
  {
    params.SetForceSearch(true);
  }
  [self fillSearchParams:params withText:searchText];
  if (!m_framework->Search(params))
  {
    g_numberOfRowsInEmptySearch = 1;
    [m_table reloadData];
  }
}

@end
