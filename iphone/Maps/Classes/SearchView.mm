
#import "SearchView.h"
#import "SearchUniversalCell.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "Statistics.h"
#import "MapViewController.h"
#import "LocationManager.h"
#import "ToastView.h"

#include "Framework.h"

#include "../../search/result.hpp"
#include "../../search/params.hpp"

#include "../../indexer/mercator.hpp"

#include "../../platform/platform.hpp"
#include "../../platform/preferred_languages.hpp"
#include "../../platform/settings.hpp"

#include "../../geometry/angles.hpp"
#include "../../geometry/distance_on_sphere.hpp"


@interface SearchResultsWrapper : NSObject

- (id)initWithResults:(search::Results const &)res;

- (search::Result const &)resultWithPosition:(NSInteger)position;
- (NSInteger)count;
- (BOOL)isEndMarker;
- (BOOL)isEndedNormal;

@end

@interface SearchResultsWrapper ()

@property (nonatomic) NSMutableDictionary * distances;

@end

@implementation SearchResultsWrapper
{
  search::Results m_results;
}

- (id)initWithResults:(search::Results const &)results
{
  self = [super init];

  m_results = results;

  return self;
}

- (NSMutableDictionary *)distances
{
  if (!_distances)
    _distances = [[NSMutableDictionary alloc] init];
  return _distances;
}

- (NSInteger)count
{
  return m_results.GetCount();
}

- (search::Result const &)resultWithPosition:(NSInteger)position
{
  return m_results.GetResult(position);
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


@interface SearchView () <UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource, SearchBarDelegate, LocationObserver, UIAlertViewDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) SolidTouchViewImageView * topBackgroundView;
@property (nonatomic) UILabel * emptyResultLabel;

- (BOOL)isShowingCategories;

@property (nonatomic) SearchResultsWrapper * wrapper;
@property (nonatomic) NSArray *categoriesNames;

@end

@implementation SearchView
{
  BOOL needToScroll;
}
__weak SearchView * selfPointer;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.tableView];
  [self addSubview:self.topBackgroundView];
  self.topBackgroundView.height = SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 44 : 64;
  [self addSubview:self.searchBar];
  [self.tableView addSubview:self.emptyResultLabel];

  self.emptyResultLabel.center = CGPointMake(self.width / 2, 40);
  self.emptyResultLabel.hidden = YES;

  self.searchBar.midX = self.width / 2;

  [self setState:SearchViewStateHidden animated:NO withCallback:NO];

  selfPointer = self;

  double latitude;
  double longitude;
  bool const hasPt = [[MapsAppDelegate theApp].m_locationManager getLat:latitude Lon:longitude];
  GetFramework().PrepareSearch(hasPt, latitude, longitude);

  [self addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];

  needToScroll = NO;

  [self.tableView registerClass:[SearchUniversalCell class] forCellReuseIdentifier:[SearchUniversalCell className]];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startMonitoringLocation:) name:LOCATION_MANAGER_STARTED_NOTIFICATION object:nil];

  return self;
}

- (void)startMonitoringLocation:(NSNotification *)notification
{
  [[MapsAppDelegate theApp].m_locationManager start:self];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if (object == self && [keyPath isEqualToString:@"state"])
    [self setState:self.state animated:YES withCallback:NO];
}

- (void)setState:(SearchViewState)state animated:(BOOL)animated withCallback:(BOOL)withCallback
{
  if (_state == SearchViewStateResults && state == SearchViewStateHidden)
    [self clearSearchResultsMode];

  UIViewAnimationOptions options = UIViewAnimationOptionCurveEaseInOut;
  double damping = 0.9;
  NSTimeInterval duration = animated ? 0.3 : 0;
  CGFloat searchBarOffset = (state == SearchViewStateHidden) ? -self.searchBar.height : [self defaultSearchBarMinY];

  CGRect fieldBackgroundFrame = CGRectMake(15, self.searchBar.fieldBackgroundView.minY, self.searchBar.width - 84, self.searchBar.fieldBackgroundView.height);
  CGRect textFieldFrame = CGRectMake(24, self.searchBar.textField.minY, self.searchBar.width - 119, 22);

  CGFloat const shift = 55;

  CGRect shiftedFieldBackgroundFrame = fieldBackgroundFrame;
  shiftedFieldBackgroundFrame.size.width += shift;

  CGRect shiftedTextFieldFrame = textFieldFrame;
  shiftedTextFieldFrame.size.width += shift;

  if (state == SearchViewStateFullscreen)
  {
    [self.searchBar.textField becomeFirstResponder];
    [UIView animateWithDuration:0.25 delay:0. options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.tableView.alpha = 1;
    } completion:^(BOOL finished) {}];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.searchBar.cancelButton.alpha = 1;
      self.topBackgroundView.minY = 0;
      self.searchBar.minY = searchBarOffset;
      self.searchBar.alpha = 1;
      self.searchBar.fieldBackgroundView.frame = fieldBackgroundFrame;
      self.searchBar.textField.frame = textFieldFrame;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    } completion:^(BOOL finished){
      if (needToScroll)
      {
        needToScroll = NO;
        [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:YES];
      }
    }];
  }
  else if (state == SearchViewStateResults)
  {
    [self.searchBar.textField resignFirstResponder];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.searchBar.cancelButton.alpha = 0;
      self.searchBar.minY = searchBarOffset;
      self.searchBar.alpha = 1;
      self.topBackgroundView.minY = 0;
      self.tableView.alpha = 0;
      self.searchBar.fieldBackgroundView.frame = shiftedFieldBackgroundFrame;
      self.searchBar.textField.frame = shiftedTextFieldFrame;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearResultsButton"] forState:UIControlStateNormal];
    } completion:nil];
  }
  else if (state == SearchViewStateHidden)
  {
    [self.searchBar.textField resignFirstResponder];
    [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.searchBar.cancelButton.alpha = 1;
      self.searchBar.maxY = 0;
      self.searchBar.alpha = 0;
      self.topBackgroundView.maxY = 0;
      self.tableView.alpha = 0;
      self.searchBar.fieldBackgroundView.frame = fieldBackgroundFrame;
      self.searchBar.textField.frame = textFieldFrame;
      [self.searchBar.clearButton setImage:[UIImage imageNamed:@"SearchBarClearButton"] forState:UIControlStateNormal];
    } completion:nil];
  }
  else if (state == SearchViewStateAlpha)
  {
    [self.searchBar.textField resignFirstResponder];
  }
  if (withCallback)
    [self willChangeValueForKey:@"state"];
  _state = state;
  if (withCallback)
    [self didChangeValueForKey:@"state"];
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error %i in %@", errorCode, self);
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  if ([self.searchBar.textField.text length] && self.state == SearchViewStateFullscreen)
  {
    search::SearchParams params = [self searchParametersWithForce:NO];
    params.SetPosition(info.m_latitude, info.m_longitude);
    GetFramework().Search(params);

    [self recalculateDistances];
    [self.tableView reloadRowsAtIndexPaths:self.tableView.indexPathsForVisibleRows withRowAnimation:UITableViewRowAnimationNone];
  }
}

- (void)recalculateDistances
{
  LocationManager * locationManager = [MapsAppDelegate theApp].m_locationManager;
  double north = -1.0;
  double azimut = -1.0;
  [locationManager getNorthRad:north];
  double lat, lon;
  if ([locationManager getLat:lat Lon:lon])
  {
    SearchResultsWrapper * wrapper = self.wrapper;
    for (NSInteger position = 0; position < [wrapper count]; position++)
    {
      search::Result const & result = [wrapper resultWithPosition:position];
      string distance;
      if (result.GetResultType() != search::Result::RESULT_SUGGESTION)
      {
        GetFramework().GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, north, distance, azimut);
        wrapper.distances[@(position)] = [NSString stringWithUTF8String:distance.c_str()];
      }
    }
  }
}

- (search::SearchParams)searchParametersWithForce:(BOOL)force
{
  search::SearchParams params;
  params.SetSearchMode(search::SearchParams::ALL);
  params.m_query = [[self.searchBar.textField.text precomposedStringWithCompatibilityMapping] UTF8String];
  params.m_callback = bind(&OnSearchResultCallback, _1);
  params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);
  params.SetForceSearch(force == YES);

  return params;
}

- (void)search
{
  self.emptyResultLabel.hidden = YES;
  [self.searchBar setSearching:YES];
  search::SearchParams params = [self searchParametersWithForce:YES];
  double lat, lon;
  if ([[MapsAppDelegate theApp].m_locationManager getLat:lat Lon:lon])
    params.SetPosition(lat, lon);
  GetFramework().Search(params);
  [self.tableView reloadData];
}

static void OnSearchResultCallback(search::Results const & results)
{
  SearchResultsWrapper * wrapper = [[SearchResultsWrapper alloc] initWithResults:results];
  [selfPointer performSelectorOnMainThread:@selector(frameworkDidAddSearchResult:) withObject:wrapper waitUntilDone:NO];
}

- (void)frameworkDidAddSearchResult:(SearchResultsWrapper *)wrapper
{
  if ([wrapper isEndMarker])
  {
    if ([wrapper isEndedNormal])
    {
      [self.searchBar setSearching:NO];
      [self recalculateDistances];
      [self.tableView reloadData];
    }
  }
  else
  {
    self.emptyResultLabel.hidden = [self isShowingCategories] ? YES : ([wrapper count] > 0);
    self.wrapper = wrapper;
    [self.tableView reloadData];
    [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:YES];
  }
}

- (void)clearSearchResultsMode
{
  Framework & framework = GetFramework();
  framework.GetBalloonManager().RemovePin();
  framework.GetBalloonManager().Dismiss();
  framework.CancelInteractiveSearch();
}

- (void)searchBarDidPressClearButton:(SearchBar *)searchBar
{
  if (self.state == SearchViewStateResults)
    [self setState:SearchViewStateHidden animated:YES withCallback:YES];
  else
    [self.searchBar.textField becomeFirstResponder];

  self.searchBar.textField.text = nil;
  [self processTextChanging];
}

- (void)searchBarDidPressCancelButton:(id)searchBar
{
  self.searchBar.textField.text = nil;
  [self.searchBar setSearching:NO];
  [self.tableView reloadData];
  self.emptyResultLabel.alpha = 0;
  [self setState:SearchViewStateHidden animated:YES withCallback:YES];
}

- (void)textFieldTextChanged:(id)sender
{
  [self processTextChanging];
}

- (void)textFieldBegin:(id)sender
{
  if (self.state == SearchViewStateResults)
    [self clearSearchResultsMode];
  [self setState:SearchViewStateFullscreen animated:YES withCallback:YES];
}

- (void)processTextChanging
{
  if ([self isShowingCategories])
  {
    self.emptyResultLabel.hidden = YES;
    [self.tableView reloadData];
  }
  else
  {
    [self search];
  }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if ([self.wrapper count] && ![self isShowingCategories])
  {
    Framework & f = GetFramework();
    if (f.ShowAllSearchResults() == 0)
    {
      NSString * message = [NSString stringWithFormat:@"%@. %@", NSLocalizedString(@"no_search_results_found", nil), NSLocalizedString(@"download_location_country", nil)];
      message = [message stringByReplacingOccurrencesOfString:@" (%@)" withString:@""];
      ToastView * toastView = [[ToastView alloc] initWithMessage:message];
      [toastView show];
    }

    search::SearchParams params;
    params.m_query = [[self.searchBar.textField.text precomposedStringWithCompatibilityMapping] UTF8String];
    params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);

    f.StartInteractiveSearch(params);

    [self setState:SearchViewStateResults animated:YES withCallback:YES];
    return YES;
  }
  return NO;
}

- (CGFloat)defaultSearchBarMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 3 : 20;
}

- (void)layoutSubviews
{
  if (self.state == SearchViewStateFullscreen)
    self.searchBar.minY = [self defaultSearchBarMinY];
  self.tableView.contentInset = UIEdgeInsetsMake(self.topBackgroundView.height, 0, 0, 0);
  self.tableView.scrollIndicatorInsets = self.tableView.contentInset;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  SearchUniversalCell * cell = [tableView dequeueReusableCellWithIdentifier:[SearchUniversalCell className]];

  if ([self isShowingCategories])
  {
    // initial categories
    cell.iconImageView.image = [UIImage imageNamed:@"SearchCellSpotIcon"];
    cell.distanceLabel.text = nil;
    cell.typeLabel.text = nil;
    [cell setTitle:NSLocalizedString(self.categoriesNames[indexPath.row], nil) selectedRange:NSMakeRange(0, 0)];
    [cell setSubtitle:nil selectedRange:NSMakeRange(0, 0)];
  }
  else
  {
    if ([self indexPathIsForSearchResultItem:indexPath])
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      SearchResultsWrapper * wrapper = self.wrapper;
      search::Result const & result = [wrapper resultWithPosition:position];

      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      NSRange titleRange = [title rangeOfString:self.searchBar.textField.text options:NSCaseInsensitiveSearch];
      [cell setTitle:title selectedRange:titleRange];

      if (result.GetResultType() == search::Result::RESULT_SUGGESTION)
      {
        // suggest item
        cell.iconImageView.image = [UIImage imageNamed:@"SearchCellSpotIcon"];
        cell.distanceLabel.text = nil;
        cell.typeLabel.text = nil;
        [cell setSubtitle:nil selectedRange:NSMakeRange(0, 0)];
      }
      else if (result.GetResultType() == search::Result::RESULT_POI_SUGGEST)
      {
        // poi suggest item
        cell.iconImageView.image = [UIImage imageNamed:@"SearchCellPinsIcon"];
        cell.distanceLabel.text = nil;
        cell.typeLabel.text = nil;
        [cell setSubtitle:nil selectedRange:NSMakeRange(0, 0)];
      }
      else
      {
        // final search result item
        cell.iconImageView.image = [UIImage imageNamed:@"SearchCellPinIcon"];
        cell.distanceLabel.text = wrapper.distances[@(position)];
        cell.typeLabel.text = [NSString stringWithUTF8String:result.GetFeatureType()];
        NSString * subtitle = [NSString stringWithUTF8String:result.GetRegionString()];
        NSRange subtitleRange = [subtitle rangeOfString:self.searchBar.textField.text options:NSCaseInsensitiveSearch];
        [cell setSubtitle:subtitle selectedRange:subtitleRange];
      }
    }
    else
    {
      // 'show on map' cell
      cell.iconImageView.image = [UIImage imageNamed:@"SearchCellSpotIcon"];
      cell.distanceLabel.text = nil;
      cell.typeLabel.text = nil;
      [cell setTitle:NSLocalizedString(@"search_show_on_map", nil) selectedRange:NSMakeRange(0, 0)];
      [cell setSubtitle:nil selectedRange:NSMakeRange(0, 0)];
    }
  }
  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isShowingCategories])
  {
    return [SearchUniversalCell cellHeightWithTitle:self.categoriesNames[indexPath.row] type:nil subtitle:nil distance:nil viewWidth:tableView.width];
  }
  else
  {
    if ([self indexPathIsForSearchResultItem:indexPath])
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      SearchResultsWrapper * wrapper = self.wrapper;
      search::Result const & result = [wrapper resultWithPosition:position];
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      NSString * subtitle = [NSString stringWithUTF8String:result.GetRegionString()];
      NSString * type = [NSString stringWithUTF8String:result.GetFeatureType()];
      return [SearchUniversalCell cellHeightWithTitle:title type:type subtitle:subtitle distance:wrapper.distances[@(position)] viewWidth:tableView.width];
    }
    else
    {
      return [SearchUniversalCell cellHeightWithTitle:NSLocalizedString(@"search_show_on_map", nil) type:nil subtitle:nil distance:nil viewWidth:tableView.width];
    }
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  if ([self isShowingCategories])
  {
    [[Statistics instance] logEvent:@"Category Selection" withParameters:@{@"Category" : self.categoriesNames[indexPath.row]}];
    self.searchBar.textField.text = [NSLocalizedString(self.categoriesNames[indexPath.row], nil) stringByAppendingString:@" "];
    [self search];
    return;
  }

  if ([self indexPathIsForSearchResultItem:indexPath])
  {
    NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
    search::Result const & result = [self.wrapper resultWithPosition:position];
    if (result.GetResultType() == search::Result::RESULT_SUGGESTION)
    {
      self.searchBar.textField.text = [NSString stringWithUTF8String:result.GetSuggestionString()];
      [self search];
    }
    else if (result.GetResultType() == search::Result::RESULT_POI_SUGGEST)
    {
      self.searchBar.textField.text = [NSString stringWithUTF8String:result.GetSuggestionString()];
      [self search];
    }
    else
    {
      if (GetPlatform().IsPro())
      {
        GetFramework().ShowSearchResult(result);
        needToScroll = YES;
        [self setState:SearchViewStateHidden animated:YES withCallback:YES];
      }
      else
      {
        UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"search_available_in_pro_version", nil) message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"cancel", nil) otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
        [alert show];
      }
    }
  }
  else
  {
    GetFramework().ShowAllSearchResults();
    needToScroll = YES;
    [self setState:SearchViewStateResults animated:YES withCallback:YES];
  }
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_search_alert"];
    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"YES"];
  }
  else
  {
    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"NO"];
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  if (!scrollView.decelerating && scrollView.dragging)
    [self.searchBar.textField resignFirstResponder];
}

- (BOOL)indexPathIsForSearchResultItem:(NSIndexPath *)indexPath
{
  return YES;//indexPath.row || [self rowsCount] == 1;
}

- (NSInteger)searchResultPositionForIndexPath:(NSIndexPath *)indexPath
{
  return indexPath.row;//([self rowsCount] == 1) ? 0 : indexPath.row - 1;
}

- (NSInteger)rowsCount
{
  SearchResultsWrapper * wrapper = self.wrapper;
  NSInteger const wrapperCount = [wrapper count];
  NSInteger resultsCount;
  if (wrapperCount)
    resultsCount = (wrapperCount == 1) ? 1 : wrapperCount + 1;
  else
    resultsCount = 0;

  resultsCount = wrapperCount;

  return [self isShowingCategories] ? [self.categoriesNames count] : resultsCount;
}

- (BOOL)isShowingCategories
{
  return ![self.searchBar.textField.text length];
}

- (NSArray *)categoriesNames
{
  if (!_categoriesNames)
    _categoriesNames = @[@"food", @"shop", @"hotel", @"tourism", @"entertainment", @"atm", @"bank", @"transport", @"fuel", @"parking", @"pharmacy", @"hospital", @"toilet", @"post", @"police"];
  return _categoriesNames;
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  return CGRectContainsPoint(self.searchBar.frame, point) || self.state == SearchViewStateFullscreen;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:self.bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.backgroundColor = [UIColor colorWithColorCode:@"414451"];
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
  }
  return _tableView;
}

- (SolidTouchViewImageView *)topBackgroundView
{
  if (!_topBackgroundView)
  {
    _topBackgroundView = [[SolidTouchViewImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
    _topBackgroundView.image = [[UIImage imageNamed:@"SearchViewTopBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 0, 10, 0) resizingMode:UIImageResizingModeStretch];
    _topBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _topBackgroundView.userInteractionEnabled = YES;
  }
  return _topBackgroundView;
}

- (SearchBar *)searchBar
{
  if (!_searchBar)
  {
    _searchBar = [[SearchBar alloc] initWithFrame:CGRectMake(0, 0, self.width, 44)];
    _searchBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _searchBar.textField.delegate = self;
    _searchBar.delegate = self;
    [_searchBar.textField addTarget:self action:@selector(textFieldTextChanged:) forControlEvents:UIControlEventEditingChanged];
    [_searchBar.textField addTarget:self action:@selector(textFieldBegin:) forControlEvents:UIControlEventEditingDidBegin];
  }
  return _searchBar;
}

- (UILabel *)emptyResultLabel
{
  if (!_emptyResultLabel)
  {
    _emptyResultLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.width, 60)];
    _emptyResultLabel.backgroundColor = [UIColor clearColor];
    _emptyResultLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _emptyResultLabel.text = NSLocalizedString(@"no_search_results_found", nil);
    _emptyResultLabel.textColor = [UIColor whiteColor];
    _emptyResultLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _emptyResultLabel.textAlignment = NSTextAlignmentCenter;
  }
  return _emptyResultLabel;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
