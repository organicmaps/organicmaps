
#import "SearchView.h"
#import "SearchUniversalCell.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "Statistics.h"
#import "MapViewController.h"
#import "LocationManager.h"
#import "ToastView.h"
#import "SearchSuggestCell.h"

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
- (NSInteger)suggestsCount;
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

- (NSInteger)suggestsCount
{
  return m_results.GetSuggestsCount();
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


typedef NS_ENUM(NSUInteger, CellType)
{
  CellTypeResult,
  CellTypeSuggest,
  CellTypeShowOnMap,
  CellTypeCategory
};


@interface SearchView () <UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource, SearchBarDelegate, LocationObserver, UIAlertViewDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) SolidTouchImageView * topBackgroundView;
@property (nonatomic) UILabel * emptyResultLabel;
@property (nonatomic) UIImageView * suggestsTopImageView;

@property (nonatomic) SearchResultsWrapper * wrapper;
@property (nonatomic) NSArray * categoriesNames;
@property (nonatomic) BOOL isShowingCategories;

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
  [self addObserver:self forKeyPath:@"state" options:NSKeyValueObservingOptionNew context:nil];

  selfPointer = self;
  needToScroll = NO;
  self.isShowingCategories = YES;

  [self.tableView registerClass:[SearchUniversalCell class] forCellReuseIdentifier:[SearchUniversalCell className]];
  [self.tableView registerClass:[SearchSuggestCell class] forCellReuseIdentifier:[SearchSuggestCell className]];

  return self;
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

  if (_state == SearchViewStateResults && state == SearchViewStateFullscreen && self.isShowingCategories)
    self.searchBar.textField.text = nil;

  UIViewAnimationOptions options = UIViewAnimationOptionCurveEaseInOut;
  double damping = 0.9;
  NSTimeInterval duration = animated ? 0.3 : 0;
  CGFloat searchBarOffset = (state == SearchViewStateHidden) ? -self.searchBar.height : [self defaultSearchBarMinY];

  CGFloat const fieldBackgroundMinX = 12.5;
  CGRect const fieldBackgroundFrame = CGRectMake(fieldBackgroundMinX, self.searchBar.fieldBackgroundView.minY, self.searchBar.width - fieldBackgroundMinX - 69, self.searchBar.fieldBackgroundView.height);

  CGFloat const textFieldMinX = 44;
  CGRect const textFieldFrame = CGRectMake(textFieldMinX, self.searchBar.textField.minY, self.searchBar.width - textFieldMinX - 95, 22);

  CGFloat const shift = 55;
  CGRect shiftedFieldBackgroundFrame = fieldBackgroundFrame;
  shiftedFieldBackgroundFrame.size.width += shift;

  CGRect shiftedTextFieldFrame = textFieldFrame;
  shiftedTextFieldFrame.size.width += shift;

  if (state == SearchViewStateFullscreen)
  {
    [[MapsAppDelegate theApp].m_locationManager start:self];

    double latitude;
    double longitude;
    bool const hasPt = [[MapsAppDelegate theApp].m_locationManager getLat:latitude Lon:longitude];
    GetFramework().PrepareSearch(hasPt, latitude, longitude);

    [self.searchBar.textField becomeFirstResponder];
    [UIView animateWithDuration:0.25 delay:0. options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.tableView.alpha = 1;
    } completion:nil];
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
    [self.tableView reloadData];
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
  params.m_callback = bind(&onSearchResultCallback, _1);
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
}

static void onSearchResultCallback(search::Results const & results)
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
    self.emptyResultLabel.hidden = self.isShowingCategories ? YES : ([self rowsCount] > 0);
    self.wrapper = wrapper;
    self.suggestsTopImageView.hidden = ![wrapper suggestsCount];
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
  self.isShowingCategories = YES;
  [self processTextChanging];
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
  self.isShowingCategories = !self.searchBar.textField.text.length;
  if (self.isShowingCategories)
  {
    [self.searchBar setSearching:NO];
    self.suggestsTopImageView.hidden = YES;
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
  if ([self.wrapper count] && !self.isShowingCategories)
  {
    if (GetPlatform().IsPro())
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
    }
    else
    {
      [self showBuyProMessage];
    }
    return YES;
  }
  return NO;
}

- (CGFloat)defaultSearchBarMinY
{
  return SYSTEM_VERSION_IS_LESS_THAN(@"7") ? 3 : 20;
}

- (void)showBuyProMessage
{
  UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"search_available_in_pro_version", nil) message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"cancel", nil) otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
  [alert show];
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
  UITableViewCell * cell;
  CellType cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case CellTypeCategory:
    {
      SearchUniversalCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchUniversalCell className]];

      [customCell setTitle:NSLocalizedString(self.categoriesNames[indexPath.row], nil) selectedRanges:nil];
      customCell.subtitleLabel.text = nil;
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchCellSpotIcon"];
      customCell.distanceLabel.text = nil;
      customCell.typeLabel.text = nil;
      cell = customCell;
      break;
    }
    case CellTypeShowOnMap:
    {
      SearchUniversalCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchUniversalCell className]];

      [customCell setTitle:NSLocalizedString(@"search_show_on_map", nil) selectedRanges:nil];
      customCell.subtitleLabel.text = nil;
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchCellPinsIcon"];
      customCell.distanceLabel.text = nil;
      customCell.typeLabel.text = nil;
      cell = customCell;
      break;
    }
    case CellTypeResult:
    {
      SearchUniversalCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchUniversalCell className]];

      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      NSMutableArray * ranges = [[NSMutableArray alloc] init];
      for (size_t i = 0; i < result.GetHighlightRangesCount(); i++)
      {
        pair<uint16_t, uint16_t> const & pairRange = result.GetHighlightRange(i);
        NSRange range = NSMakeRange(pairRange.first, pairRange.second);
        [ranges addObject:[NSValue valueWithRange:range]];
      }
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      [customCell setTitle:title selectedRanges:ranges];
      customCell.subtitleLabel.text = [NSString stringWithUTF8String:result.GetRegionString()];
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchCellPinIcon"];
      customCell.distanceLabel.text = self.wrapper.distances[@(position)];
      customCell.typeLabel.text = [NSString stringWithUTF8String:result.GetFeatureType()];
      cell = customCell;
      break;
    }
    case CellTypeSuggest:
    {
      SearchSuggestCell * customCell = [tableView dequeueReusableCellWithIdentifier:[SearchSuggestCell className]];

      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];

      customCell.titleLabel.text = [NSString stringWithUTF8String:result.GetString()];
      customCell.iconImageView.image = [UIImage imageNamed:@"SearchCellSpotIcon"];
      customCell.position = [self suggestPositionForIndexPath:indexPath];
      cell = customCell;
      break;
    }
  }
  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellType cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case CellTypeCategory:
    {
      return [SearchUniversalCell cellHeightWithTitle:self.categoriesNames[indexPath.row] type:nil subtitle:nil distance:nil viewWidth:tableView.width];
    }
    case CellTypeShowOnMap:
    {
      return [SearchUniversalCell cellHeightWithTitle:NSLocalizedString(@"search_show_on_map", nil) type:nil subtitle:nil distance:nil viewWidth:tableView.width];
    }
    case CellTypeResult:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      SearchResultsWrapper * wrapper = self.wrapper;
      search::Result const & result = [wrapper resultWithPosition:position];
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      NSString * subtitle;
      NSString * type;
      if (result.GetResultType() == search::Result::RESULT_FEATURE || result.GetResultType() == search::Result::RESULT_LATLON)
      {
        subtitle = [NSString stringWithUTF8String:result.GetRegionString()];
        type = [NSString stringWithUTF8String:result.GetFeatureType()];
      }
      return [SearchUniversalCell cellHeightWithTitle:title type:type subtitle:subtitle distance:wrapper.distances[@(position)] viewWidth:tableView.width];
    }
    case CellTypeSuggest:
    {
      return [SearchSuggestCell cellHeightWithPosition:[self suggestPositionForIndexPath:indexPath]];
    }
    default:
    {
      return 0;
    }
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (void)startInteractiveSearch
{
  search::SearchParams params;
  params.m_query = [[self.searchBar.textField.text precomposedStringWithCompatibilityMapping] UTF8String];
  params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);

  Framework & f = GetFramework();
  f.StartInteractiveSearch(params);
  f.UpdateUserViewportChanged();
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];

  CellType cellType = [self cellTypeForIndexPath:indexPath];

  switch (cellType)
  {
    case CellTypeCategory:
    {
      if (GetPlatform().IsPro())
      {
        [[Statistics instance] logEvent:@"Category Selection" withParameters:@{@"Category" : self.categoriesNames[indexPath.row]}];
        self.searchBar.textField.text = [NSLocalizedString(self.categoriesNames[indexPath.row], nil) stringByAppendingString:@" "];
        [self setState:SearchViewStateResults animated:YES withCallback:YES];
        self.isShowingCategories = YES;
        [self startInteractiveSearch];
      }
      else
      {
        [self showBuyProMessage];
      }

      break;
    }
    case CellTypeShowOnMap:
    {
      if (GetPlatform().IsPro())
      {
        [self setState:SearchViewStateResults animated:YES withCallback:YES];
        [self startInteractiveSearch];
      }
      else
      {
        [self showBuyProMessage];
      }

      break;
    }
    case CellTypeResult:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      if (GetPlatform().IsPro())
      {
        [self setState:SearchViewStateHidden animated:YES withCallback:YES];
        GetFramework().ShowSearchResult(result);
      }
      else
      {
        [self showBuyProMessage];
      }

      break;
    }
    case CellTypeSuggest:
    {
      NSInteger const position = [self searchResultPositionForIndexPath:indexPath];
      search::Result const & result = [self.wrapper resultWithPosition:position];
      self.searchBar.textField.text = [NSString stringWithUTF8String:result.GetSuggestionString()];
      [self search];

      break;
    }
  }
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == alertView.cancelButtonIndex)
  {
    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"NO"];
  }
  else
  {
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_search_alert"];
    [[Statistics instance] logProposalReason:@"Search Screen" withAnswer:@"YES"];
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  if (!scrollView.decelerating && scrollView.dragging)
    [self.searchBar.textField resignFirstResponder];
}

- (SearchSuggestCellPosition)suggestPositionForIndexPath:(NSIndexPath *)indexPath
{
  return (indexPath.row == [self.wrapper suggestsCount] - 1) ? SearchSuggestCellPositionBottom : SearchSuggestCellPositionMiddle;
}

- (CellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  if (self.isShowingCategories)
  {
    return CellTypeCategory;
  }
  else
  {
    if ([self.wrapper suggestsCount])
      return indexPath.row < [self.wrapper suggestsCount] ? CellTypeSuggest : CellTypeResult;
    else
      return indexPath.row == 0 ? CellTypeShowOnMap : CellTypeResult;
  }
}

- (NSInteger)searchResultPositionForIndexPath:(NSIndexPath *)indexPath
{
  return [self.wrapper suggestsCount] ? indexPath.row : indexPath.row - 1;
}

- (NSInteger)rowsCount
{
  if (self.isShowingCategories)
    return [self.categoriesNames count];
  else
    return [self.wrapper suggestsCount] ? [self.wrapper count] : [self.wrapper count] + 1;
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
    [_tableView addSubview:self.suggestsTopImageView];
    self.suggestsTopImageView.maxY = 0;
  }
  return _tableView;
}

- (UIImageView *)suggestsTopImageView
{
  if (!_suggestsTopImageView)
  {
    UIImage * image = [[UIImage imageNamed:@"SearchSuggestBackgroundMiddle"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 40, 10, 40)];
    _suggestsTopImageView = [[UIImageView alloc] initWithImage:image];
    _suggestsTopImageView.frame = CGRectMake(0, 0, self.width, 600);
    _suggestsTopImageView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _suggestsTopImageView.hidden = YES;
  }
  return _suggestsTopImageView;
}

- (SolidTouchImageView *)topBackgroundView
{
  if (!_topBackgroundView)
  {
    _topBackgroundView = [[SolidTouchImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
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
