
#import "SearchView.h"
#import "SegmentedControl.h"
#import "SearchUniversalCell.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "LocationManager.h"
#import "Statistics.h"
#import "MapViewController.h"
#import "LocationManager.h"

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

@property (nonatomic) search::Results results;
@property (nonatomic) NSMutableDictionary * distances;

@end

@implementation SearchResultsWrapper

- (id)initWithResults:(search::Results const &)results
{
  self = [super init];

  self.results = results;

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
  return self.results.GetCount();
}

- (search::Result const &)resultWithPosition:(NSInteger)position
{
  return self.results.GetResult(position);
}

- (BOOL)isEndMarker
{
  return self.results.IsEndMarker();
}

- (BOOL)isEndedNormal
{
  return self.results.IsEndedNormal();
}

@end


@interface SearchView () <UITextFieldDelegate, UITableViewDelegate, UITableViewDataSource, SearchBarDelegate, SegmentedControlDelegate, LocationObserver>

@property (nonatomic) SegmentedControl * segmentedControl;
@property (nonatomic) UITableView * tableView;
@property (nonatomic) UIView * backgroundView;
@property (nonatomic) UIImageView * topBackgroundView;
@property (nonatomic) UILabel * emptyResultLabel;

- (BOOL)isShowingCategories;

@property (nonatomic) NSMutableArray * searchData;
@property (nonatomic) NSArray *categoriesNames;

@end

@implementation SearchView
@synthesize active = _active;

__weak SearchView * selfPointer;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  [self addSubview:self.backgroundView];
  [self addSubview:self.tableView];
  [self addSubview:self.topBackgroundView];
  [self addSubview:self.segmentedControl];
  [self addSubview:self.searchBar];
  [self addSubview:self.emptyResultLabel];

  self.emptyResultLabel.center = CGPointMake(self.width / 2, 160);
  self.emptyResultLabel.hidden = YES;

  self.searchBar.midX = self.width / 2;
  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(barTapped:)];
  [self.searchBar addGestureRecognizer:tap];

  NSInteger value;
  self.segmentedControl.selectedSegmentIndex = Settings::Get("SearchMode", value) ? value : 0;
  self.segmentedControl.midX = self.width / 2;

  [self setActive:NO animated:NO];

  selfPointer = self;

  double latitude;
  double longitude;
  LocationManager * locationManager = [MapsAppDelegate theApp].m_locationManager;
  bool const hasPt = [locationManager getLat:latitude Lon:longitude];
  GetFramework().PrepareSearch(hasPt, latitude, longitude);

  [locationManager start:self];

  return self;
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error in SearchView");
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  if ([self.searchBar.textField.text length])
  {
    search::SearchParams params = [self searchParameters];
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
    for (NSInteger segment = 0; segment < [self.segmentedControl segmentsCount]; segment++)
    {
      SearchResultsWrapper * wrapper = self.searchData[segment];
      for (NSInteger position = 0; position < [wrapper count]; position++)
      {
        search::Result const & result = [wrapper resultWithPosition:position];
        string distance;
        GetFramework().GetDistanceAndAzimut(result.GetFeatureCenter(), lat, lon, north, distance, azimut);
        wrapper.distances[@(position)] = [NSString stringWithUTF8String:distance.c_str()];
      }
    }
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{

}

- (search::SearchParams)searchParameters
{
  NSInteger scopeIndex = self.segmentedControl.selectedSegmentIndex;
  search::SearchParams params;
  if (scopeIndex == 0)
    params.SetSearchMode(search::SearchParams::AROUND_POSITION);
  else if (scopeIndex == 1)
    params.SetSearchMode(search::SearchParams::IN_VIEWPORT);
  else if (scopeIndex == 2)
    params.SetSearchMode(search::SearchParams::ALL);

  params.m_query = [[self.searchBar.textField.text precomposedStringWithCompatibilityMapping] UTF8String];
  params.m_callback = bind(&OnSearchResultCallback, _1);
  params.SetInputLanguage([[UITextInputMode currentInputMode].primaryLanguage UTF8String]);
  params.SetForceSearch(true);

  return params;
}

- (void)search
{
  self.emptyResultLabel.hidden = YES;
  [self.searchBar setSearching:YES];
  search::SearchParams params = [self searchParameters];
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
    self.searchData[self.segmentedControl.selectedSegmentIndex] = wrapper;
    [self.tableView reloadData];
    [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:YES];
  }
}

- (void)searchBarDidPressClearButton:(SearchBar *)searchBar
{
  [self processTextChanging];
}

- (void)searchBarDidPressCancelButton:(id)searchBar
{
  [self setActive:NO animated:YES];
}

- (void)setActive:(BOOL)active animated:(BOOL)animated
{
  [self.searchBar setActive:active animated:animated];
  [self.segmentedControl setActive:active animated:animated];
  if (active)
  {
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:0.9 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.backgroundView.alpha = 1;
      self.topBackgroundView.alpha = 1;
      self.tableView.alpha = 1;
      self.tableView.minY = 0;
    } completion:^(BOOL finished){
      [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:YES];
    }];
  }
  else
  {
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:0.9 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
      self.backgroundView.alpha = 0;
      self.topBackgroundView.alpha = 0;
      self.tableView.alpha = 0;
      self.tableView.minY = self.height;
    } completion:nil];
  }
  [self willChangeValueForKey:@"active"];
  _active = active;
  [self didChangeValueForKey:@"active"];
}

- (void)barTapped:(UITapGestureRecognizer *)sender
{
  [self setActive:YES animated:YES];
}

- (void)textFieldTextChanged:(id)sender
{
  [self processTextChanging];
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
  return [textField.text length] > 0;
}

- (void)segmentedControl:(SegmentedControl *)segmentControl didSelectSegment:(NSInteger)segmentIndex
{
  Settings::Set("SearchMode", (int)segmentIndex);
  [self search];
}

- (CGFloat)defaultSearchBarMinY
{
  if (SYSTEM_VERSION_IS_LESS_THAN(@"7"))
    return self.width < self.height ? 10 : 4;
  else
    return self.width < self.height ? 30 : 20;
}

- (void)layoutSubviews
{
  self.searchBar.minY = [self defaultSearchBarMinY];
  if (self.width < self.height)
  {
    self.segmentedControl.minY = self.searchBar.maxY - 4;
    self.segmentedControl.height = 40;
    self.topBackgroundView.height = self.segmentedControl.maxY + 3;
  }
  else
  {
    self.segmentedControl.minY = self.searchBar.maxY - 6;
    self.segmentedControl.height = 27;
    self.topBackgroundView.height = self.segmentedControl.maxY + 1;
  }
  self.tableView.contentInset = UIEdgeInsetsMake(self.topBackgroundView.maxY + 13.5, 0, 14, 0);
  self.tableView.scrollIndicatorInsets = UIEdgeInsetsMake(self.topBackgroundView.maxY, 0, 0, 0);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  SearchUniversalCell * cell = [tableView dequeueReusableCellWithIdentifier:[SearchUniversalCell className]];
  if (!cell)
    cell = [[SearchUniversalCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[SearchUniversalCell className]];

  if ([self isShowingCategories])
  {
    cell.titleLabel.text = NSLocalizedString(self.categoriesNames[indexPath.row], nil);
    cell.subtitleLabel.text = nil;
    cell.distanceLabel.text = nil;
  }
  else
  {
    if (indexPath.row == 0)
    {
      cell.titleLabel.text = @"Показать все";
      cell.subtitleLabel.text = nil;
      cell.distanceLabel.text = nil;
    }
    else
    {
      SearchResultsWrapper * wrapper = self.searchData[self.segmentedControl.selectedSegmentIndex];
      NSInteger position = indexPath.row - 1;
      search::Result const & result = [wrapper resultWithPosition:position];
      cell.titleLabel.text = [NSString stringWithUTF8String:result.GetString()];
      cell.subtitleLabel.text = result.GetRegionString() ? [NSString stringWithUTF8String:result.GetRegionString()] : nil;
      cell.distanceLabel.text = wrapper.distances[@(position)];
    }
  }

  if ([self rowsCount] == 1)
    cell.position = SearchCellPositionAlone;
  else if (indexPath.row == 0)
    cell.position = SearchCellPositionFirst;
  else if (indexPath.row == [self rowsCount] - 1)
    cell.position = SearchCellPositionLast;
  else
    cell.position = SearchCellPositionMiddle;

  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if ([self isShowingCategories])
  {
    return [SearchUniversalCell cellHeightWithTitle:self.categoriesNames[indexPath.row] subtitle:nil distance:nil viewWidth:tableView.width];
  }
  else
  {
    if (indexPath.row == 0)
    {
      return [SearchUniversalCell cellHeightWithTitle:@"Показать все" subtitle:nil distance:nil viewWidth:tableView.width];
    }
    else
    {
      SearchResultsWrapper * wrapper = self.searchData[self.segmentedControl.selectedSegmentIndex];
      NSInteger position = indexPath.row - 1;
      search::Result const & result = [wrapper resultWithPosition:position];
      NSString * title = [NSString stringWithUTF8String:result.GetString()];
      NSString * subtitle = [NSString stringWithUTF8String:result.GetRegionString()];
      return [SearchUniversalCell cellHeightWithTitle:title subtitle:subtitle distance:wrapper.distances[@(position)] viewWidth:tableView.width];
    }
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (NSInteger)rowsCount
{
  SearchResultsWrapper * wrapper = self.searchData[self.segmentedControl.selectedSegmentIndex];
  NSInteger resultsCount = [wrapper count] ? [wrapper count] + 1 : 0;
  return [self isShowingCategories] ? [self.categoriesNames count] : resultsCount;
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

  if (indexPath.row == 0)
  {
    GetFramework().ShowAllSearchResults();
    self.searchBar.resultText = self.searchBar.textField.text;
    [self setActive:NO animated:YES];
  }
  else
  {
    NSInteger segmentIndex = self.segmentedControl.selectedSegmentIndex;
    NSInteger position = indexPath.row - 1;
    search::Result const & result = [self.searchData[segmentIndex] resultWithPosition:position];
    if (result.GetResultType() == search::Result::RESULT_SUGGESTION)
    {
      self.searchBar.textField.text = [NSString stringWithUTF8String:result.GetSuggestionString()];
      [self search];
      return;
    }
    else
    {
      GetFramework().ShowSearchResult(result);

      if (segmentIndex == 0)
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"Near Me"}];
      else if (segmentIndex == 1)
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"On the Screen"}];
      else
        [[Statistics instance] logEvent:@"Search Filter" withParameters:@{@"Filter Name" : @"Everywhere"}];

      self.searchBar.resultText = [NSString stringWithUTF8String:result.GetString()];
      [self setActive:NO animated:YES];
    }
  }
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  if (!scrollView.decelerating && scrollView.dragging)
    [self.searchBar.textField resignFirstResponder];
}

- (BOOL)isShowingCategories
{
  return ![self.searchBar.textField.text length];
}

- (NSMutableArray *)searchData
{
  if (!_searchData)
    _searchData = [@[[[SearchResultsWrapper alloc] init], [[SearchResultsWrapper alloc] init], [[SearchResultsWrapper alloc] init]] mutableCopy];
  return _searchData;
}

- (NSArray *)categoriesNames
{
  if (!_categoriesNames)
    _categoriesNames = @[@"food", @"shop", @"hotel", @"tourism", @"entertainment", @"atm", @"bank", @"transport", @"fuel", @"parking", @"pharmacy", @"hospital", @"toilet", @"post", @"police",];
  return _categoriesNames;
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  return CGRectContainsPoint(self.searchBar.frame, point) || self.active;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:self.bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.backgroundColor = [UIColor clearColor];
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
  }
  return _tableView;
}

- (UIImageView *)topBackgroundView
{
  if (!_topBackgroundView)
  {
    _topBackgroundView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
    _topBackgroundView.image = [[UIImage imageNamed:@"SearchViewTopBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 0, 10, 0)];
    _topBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _topBackgroundView.userInteractionEnabled = YES;
  }
  return _topBackgroundView;
}

- (UIView *)backgroundView
{
  if (!_backgroundView)
  {
    _backgroundView = [[UIView alloc] initWithFrame:self.bounds];
    _backgroundView.backgroundColor = [UIColor colorWithColorCode:@"efeff4"];
    _backgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return _backgroundView;
}

- (SegmentedControl *)segmentedControl
{
  if (!_segmentedControl)
  {
    _segmentedControl = [[SegmentedControl alloc] initWithFrame:CGRectMake(0, 0, 294, 0)];
    _segmentedControl.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleRightMargin;
    _segmentedControl.delegate = self;
  }
  return _segmentedControl;
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
  }
  return _searchBar;
}

- (UILabel *)emptyResultLabel
{
  if (!_emptyResultLabel)
  {
    _emptyResultLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.width, 60)];
    _emptyResultLabel.backgroundColor = [UIColor clearColor];
    _emptyResultLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:14];
    _emptyResultLabel.text = @"Нет результатов";
    _emptyResultLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
    _emptyResultLabel.textAlignment = NSTextAlignmentCenter;
  }
  return _emptyResultLabel;
}

@end
