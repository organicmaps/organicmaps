#import "SearchVC.h"
#import "CompassView.h"
#import "LocationManager.h"
#import "SearchCell.h"
#import "BookmarksVC.h"
#import "CustomNavigationView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"

#include "Framework.h"

#include "../../search/result.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"


#define MAPSWITHME_PREMIUM_APPSTORE_URL @"itms://itunes.com/apps/mapswithmepro"

/// When to display compass instead of country flags
#define MIN_COMPASS_DISTANCE 25000.0


SearchVC * g_searchVC = nil;

@interface ResultsWrapper : NSObject
{
  vector<search::Result> m_results;
  NSString * m_searchString;
}

// Stores search string which corresponds to these results.
@property(nonatomic, retain) NSString * m_searchString;

- (id)initWithResults:(search::Results const &)res;
- (vector<search::Result> const &)get;
@end

@implementation ResultsWrapper

@synthesize m_searchString;

- (void)dealloc
{
  [m_searchString release];
  [super dealloc];
}

- (id)initWithResults:(search::Results const &)res
{
  if ((self = [super init]))
    m_results.assign(res.Begin(), res.End());
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

static void OnSearchResultCallback(search::Results const & res)
{
  if (g_searchVC)
  {
    ResultsWrapper * w = [[ResultsWrapper alloc] initWithResults:res];
    [g_searchVC performSelectorOnMainThread:@selector(addResult:)
                                 withObject:w waitUntilDone:NO];
    [w release];
  }
}

/////////////////////////////////////////////////////////////////////

@implementation SearchVC

- (id) init
{
  if ((self = [super initWithNibName:nil bundle:nil]))
  {
    m_framework = &GetFramework();
    m_locationManager = [MapsAppDelegate theApp].m_locationManager;
    
    double lat, lon;
    bool const hasPt = [m_locationManager getLat:lat Lon:lon];
    m_framework->PrepareSearch(hasPt, lat, lon);
  }
  return self;
}

- (void)hideIndicator
{
  [m_indicator stopAnimating];
  m_searchTextField.leftView = m_originalIndicatorView;
}

- (void)showIndicator
{
  m_searchTextField.leftView = m_indicator;
  [m_indicator startAnimating];
}

- (void)fillSearchParams:(search::SearchParams &)params withText:(NSString *)queryString
{
  params.m_query = [[queryString precomposedStringWithCompatibilityMapping] UTF8String];
  params.m_callback = bind(&OnSearchResultCallback, _1);

  // Set current keyboard input mode
  string lang;
  // UITextInputMode appeared only in iOS >= 4.2
  if (NSClassFromString(@"UITextInputMode") != nil 
      && [UITextInputMode respondsToSelector:@selector(currentInputMode)])
    lang = [[UITextInputMode currentInputMode].primaryLanguage UTF8String];
  else
    lang = languages::CurrentLanguage(); // Use current UI language instead
  params.SetInputLanguage(lang);

  double lat, lon;
  if ([m_locationManager getLat:lat Lon:lon])
    params.SetPosition(lat, lon);
}

- (void)loadView
{
  // create user interface
  // Custom view is used to automatically layout all elements
  UIView * parentView = [[[CustomNavigationView alloc] init] autorelease];
  parentView.autoresizingMask = UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;

  UINavigationBar * navBar = [[[UINavigationBar alloc] init] autorelease];
  navBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  UINavigationItem * item = [[[UINavigationItem alloc] init] autorelease];
  UIBarButtonItem * closeButton = [[[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"maps", @"Search Results - Close search button") style: UIBarButtonItemStyleDone
                                                                   target:self action:@selector(onCloseButton:)] autorelease];
  item.leftBarButtonItem = closeButton;

  m_searchBar = [[UISearchBar alloc] init];
  m_searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  // restore previous search query
  if (g_lastSearchResults)
    m_searchBar.text = g_lastSearchResults.m_searchString;
  m_searchBar.delegate = self;
  m_searchBar.placeholder = NSLocalizedString(@"search_map", @"Search box placeholder text");
  item.titleView = m_searchBar;

  // Add search in progress indicator
  for(UIView * v in m_searchBar.subviews)
  {
    if ([v isKindOfClass:[UITextField class]])
    {
      // Save textField to show/hide activity indicator in it
      m_searchTextField = (UITextField *)v;
      m_originalIndicatorView = [m_searchTextField.leftView retain];
      m_indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
      m_indicator.bounds = m_originalIndicatorView.bounds;
      break;
    }
  }

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
  [m_indicator release]; m_indicator = nil;
  [m_originalIndicatorView release]; m_originalIndicatorView = nil;
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
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:MAPSWITHME_PREMIUM_APPSTORE_URL]];
  }
  // Close Search view
  [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)IsProVersion
{
  return GetPlatform().IsPro();
}

- (void)viewWillAppear:(BOOL)animated
{
  // Disable search for free version
  if (![self IsProVersion])
  {
    // Display banner for paid version
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"search_available_in_pro_version", @"Search button pressed dialog title in the free version")
                                                     message:nil
                                                    delegate:self
                                           cancelButtonTitle:NSLocalizedString(@"cancel", @"Search button pressed dialog Negative button in the free version")
                                           otherButtonTitles:NSLocalizedString(@"get_it_now", @"Search button pressed dialog Positive button in the free version"), nil];
    [alert show];
    [alert release];
  }
  else
  {
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
  // Search even with empty string.
  search::SearchParams params;
  [self fillSearchParams:params withText:searchText];
  [self showIndicator];
  m_framework->Search(params);
}

- (void)onCloseButton:(id)sender
{
  [self dismissModalViewControllerAnimated:YES];
}
//*********** End of SearchBar handlers *************************************
//***************************************************************************

- (void)setSearchBoxText:(NSString *)text
{
  m_searchBar.text = text;
  // Manually send text change notification if control has no focus
  if (![m_searchBar isFirstResponder])
    [self searchBar:m_searchBar textDidChange:text];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  m_suggestionsCount = [m_searchBar.text length] ? 0 : 1;
  if (g_lastSearchResults)
    return [g_lastSearchResults get].size() + m_suggestionsCount;
  else
    return 0 + m_suggestionsCount;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSInteger realRowIndex = indexPath.row;

  if (m_suggestionsCount)
  {
    // Return cell with suggestion icons
    if (indexPath.row == 0)
    {
      SearchSuggestionsCell * cell = (SearchSuggestionsCell *)[tableView dequeueReusableCellWithIdentifier:@"SuggestionsCell"];
      if (!cell)
      {
        cell = [[[SearchSuggestionsCell alloc] initWithReuseIdentifier:@"SuggestionsCell"] autorelease];
        cell.delegate = self;
        [cell addIcon:[UIImage imageNamed:@"food.png"] withSuggestion:NSLocalizedString(@"food", @"Search Suggestion")];
        [cell addIcon:[UIImage imageNamed:@"money.png"] withSuggestion:NSLocalizedString(@"money", @"Search Suggestion")];
        [cell addIcon:[UIImage imageNamed:@"fuel.png"] withSuggestion:NSLocalizedString(@"fuel", @"Search Suggestion")];
        [cell addIcon:[UIImage imageNamed:@"shop.png"] withSuggestion:NSLocalizedString(@"shop", @"Search Suggestion")];
        [cell addIcon:[UIImage imageNamed:@"transport.png"] withSuggestion:NSLocalizedString(@"transport", @"Search Suggestion")];
        [cell addIcon:[UIImage imageNamed:@"tourism.png"] withSuggestion:NSLocalizedString(@"tourism", @"Search Suggestion")];
      }
      return cell;
    }
    // We're displaying one additional row with suggestions, fix results array indexing
    realRowIndex -= m_suggestionsCount;
  }

  if (g_lastSearchResults == nil || realRowIndex >= (NSInteger)[g_lastSearchResults get].size())
  {
    ASSERT(false, ("Invalid m_results with size", [g_lastSearchResults get].size()));
    return nil;
  }

  search::Result const & r = [g_lastSearchResults get][realRowIndex];
  switch (r.GetResultType())
  {
    case search::Result::RESULT_FEATURE:
    {
      SearchCell * cell = (SearchCell *)[tableView dequeueReusableCellWithIdentifier:@"FeatureCell"];
      if (!cell)
        cell = [[[SearchCell alloc] initWithReuseIdentifier:@"FeatureCell"] autorelease];

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
        UIImageView * imgView = [[[UIImageView alloc] initWithImage:flagImage] autorelease];
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
          [compass release];
        }

        // Show arrow for valid azimut and if feature is not a continent (flag is exist)
        compass.showArrow = (azimut >= 0.0 && flag) ? YES : NO;
        compass.angle = azimut;
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
  NSInteger realRowIndex = indexPath.row;
  // Suggestion cell was clicked
  if (m_suggestionsCount)
  {
    if (realRowIndex == 0)
      return;
    realRowIndex -= m_suggestionsCount;
  }

  if (realRowIndex < (NSInteger)[g_lastSearchResults get].size())
  {
    search::Result const & res = [g_lastSearchResults get][realRowIndex];
    switch(res.GetResultType())
    {
      // Zoom to the feature
    case search::Result::RESULT_FEATURE:
      {
        m_framework->ShowSearchResult(res);

        Framework::AddressInfo info;
        info.MakeFrom(res);

        [[MapsAppDelegate theApp].m_mapViewController showSearchResultAsBookmarkAtMercatorPoint:res.GetFeatureCenter() withInfo:info];

        [self onCloseButton:nil];
      }
      break;

    case search::Result::RESULT_SUGGESTION:
      [self setSearchBoxText:[NSString stringWithUTF8String:res.GetSuggestionString()]];
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
  
  [g_lastSearchResults release];
  g_lastSearchResults = [w retain];

  w.m_searchString = m_searchBar.text;

  [self hideIndicator];
  [m_table reloadData];
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
  NSString * queryString = m_searchBar.text;
  // Search even with empty string.
  //if (queryString.length)
  {
    search::SearchParams params;
    [self fillSearchParams:params withText:queryString];

    [self showIndicator];
    m_framework->Search(params);
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  double lat, lon;
  if (![m_locationManager getLat:lat Lon:lon])
    return;

  double const northRad = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
  NSArray * cells = [m_table visibleCells];
  for (NSUInteger i = 0; i < cells.count; ++i)
  {
    UITableViewCell * cell = (UITableViewCell *)[cells objectAtIndex:i];
    NSInteger realRowIndex = [m_table indexPathForCell:cell].row;
    if (m_suggestionsCount)
    {
      // Take into an account additional suggestions cell
      if (realRowIndex == 0)
        continue;
      realRowIndex -= m_suggestionsCount;
    }

    search::Result const & res = [g_lastSearchResults get][realRowIndex];
    if (res.GetResultType() == search::Result::RESULT_FEATURE)
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

// Callback from suggestion cell, called when icon is selected
- (void)onSuggestionSelected:(NSString *)suggestion
{
  [self setSearchBoxText:[suggestion stringByAppendingString:@" "]];
  // Clear old results immediately after click
  [m_table reloadData];
}

@end
