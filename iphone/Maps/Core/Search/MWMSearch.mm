#import "MWMSearch.h"
#import <Crashlytics/Crashlytics.h>
#import "MWMBannerHelpers.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "partners_api/ads_engine.hpp"

#include "search/everywhere_search_params.hpp"
#include "search/viewport_search_params.hpp"

extern NSString * const kCianCategory;

namespace
{
using Observer = id<MWMSearchObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMSearch ()

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) BOOL searchOnMap;

@property(nonatomic) BOOL textChanged;

@property(nonatomic) Observers * observers;

@property(nonatomic) NSUInteger lastSearchStamp;

@property(nonatomic) BOOL isHotelResults;
@property(nonatomic) MWMSearchFilterViewController * filter;

@property(nonatomic) BOOL everywhereSearchCompleted;
@property(nonatomic) BOOL viewportSearchCompleted;

@property(nonatomic) BOOL viewportResultsEmpty;

@property(nonatomic) MWMSearchIndex * itemsIndex;

@property(nonatomic) MWMSearchBanners * banners;

@property(nonatomic) NSInteger searchCount;

@property(copy, nonatomic) NSString * lastQuery;

@end

@implementation MWMSearch
{
  search::EverywhereSearchParams m_everywhereParams;
  search::ViewportSearchParams m_viewportParams;
  search::Results m_everywhereResults;
  vector<bool> m_isLocalAdsCustomer;
  string m_filterQuery;
}

#pragma mark - Instance

+ (MWMSearch *)manager
{
  static MWMSearch * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

+ (BOOL)isCianSearch:(NSString *)query
{
  return [query isEqualToString:[L(kCianCategory) stringByAppendingString:@" "]];
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
    _observers = [Observers weakObjectsHashTable];
  return self;
}

- (void)updateCallbacks
{
  NSUInteger const timestamp = ++self.lastSearchStamp;
  {
    __weak auto weakSelf = self;
    m_everywhereParams.m_onResults = [weakSelf, timestamp](
        search::Results const & results, vector<bool> const & isLocalAdsCustomer) {
      __strong auto self = weakSelf;
      if (!self)
        return;
      if (timestamp == self.lastSearchStamp)
      {
        self->m_everywhereResults = results;
        self->m_isLocalAdsCustomer = isLocalAdsCustomer;
        self.suggestionsCount = results.GetSuggestsCount();

        if (results.IsEndMarker())
        {
          [self checkIsHotelResults:results];
          if (results.IsEndedNormal())
          {
            self.everywhereSearchCompleted = YES;
            if ([MWMSearch isCianSearch:@(m_viewportParams.m_query.c_str())])
            {
              self.searchCount -= 1;
              return;
            }

            auto & f = GetFramework();
            if (IPAD || self.searchOnMap)
              f.ShowSearchResults(m_everywhereResults);
            f.SearchInViewport(m_viewportParams);
          }
        }
        else
        {
          [self onSearchResultsUpdated];
        }
      }
      if (results.IsEndMarker())
        self.searchCount -= 1;
    };
  }
  {
    __weak auto weakSelf = self;
    m_viewportParams.m_onStarted = [weakSelf] {
      __strong auto self = weakSelf;
      if (!self)
        return;
      self.searchCount += 1;
    };
  }
  {
    __weak auto weakSelf = self;
    m_viewportParams.m_onCompleted = [weakSelf](search::Results const & results) {
      __strong auto self = weakSelf;
      if (!self)
        return;
      if (results.IsEndedNormal())
      {
        [self checkIsHotelResults:results];
        self.viewportResultsEmpty = results.GetCount() == 0;
        self.viewportSearchCompleted = YES;
      }
      if (results.IsEndMarker())
        self.searchCount -= 1;
    };
  }
}

- (void)searchEverywhere
{
  GetFramework().SearchEverywhere(m_everywhereParams);
  self.searchCount += 1;
}

- (void)checkIsHotelResults:(search::Results const &)results
{
  self.isHotelResults = search::HotelsClassifier::IsHotelResults(results);
  m_filterQuery = m_everywhereParams.m_query;
}

- (void)updateFilters
{
  shared_ptr<search::hotels_filter::Rule> const hotelsRules = self.filter ? [self.filter rules] : nullptr;
  m_viewportParams.m_hotelsFilter = hotelsRules;
  m_everywhereParams.m_hotelsFilter = hotelsRules;
}

- (void)update
{
  [MWMSearch reset];
  if (m_everywhereParams.m_query.empty())
    return;
  [self updateCallbacks];
  [self updateFilters];
  auto & f = GetFramework();
  if ([MWMSearch isCianSearch:@(m_everywhereParams.m_query.c_str())])
    f.SearchInViewport(m_viewportParams);
  [self searchEverywhere];
}

#pragma mark - Add/Remove Observers

+ (void)addObserver:(id<MWMSearchObserver>)observer
{
  [[MWMSearch manager].observers addObject:observer];
}

+ (void)removeObserver:(id<MWMSearchObserver>)observer
{
  [[MWMSearch manager].observers removeObject:observer];
}

#pragma mark - Methods

+ (void)saveQuery:(NSString *)query forInputLocale:(NSString *)inputLocale
{
  if (!query || query.length == 0)
    return;
  CLS_LOG(@"Save search text: %@\nInputLocale: %@", query, inputLocale);
  string const locale = (!inputLocale || inputLocale.length == 0)
                            ? [MWMSearch manager]->m_everywhereParams.m_inputLocale
                            : inputLocale.UTF8String;
  string const text = query.precomposedStringWithCompatibilityMapping.UTF8String;
  GetFramework().SaveSearchQuery(make_pair(locale, text));
}

+ (void)searchQuery:(NSString *)query forInputLocale:(NSString *)inputLocale
{
  if (!query)
    return;
  CLS_LOG(@"Search text: %@\nInputLocale: %@", query, inputLocale);
  MWMSearch * manager = [MWMSearch manager];
  if (inputLocale.length != 0)
  {
    string const locale = inputLocale.UTF8String;
    manager->m_everywhereParams.m_inputLocale = locale;
    manager->m_viewportParams.m_inputLocale = locale;
  }
  manager.lastQuery = query.precomposedStringWithCompatibilityMapping;
  string const text = manager.lastQuery.UTF8String;
  manager->m_everywhereParams.m_query = text;
  manager->m_viewportParams.m_query = text;
  manager.textChanged = YES;
  auto const & adsEngine = GetFramework().GetAdsEngine();
  if (![MWMSettings adForbidden] && adsEngine.HasSearchBanner())
  {
    auto coreBanners = banner_helpers::MatchPriorityBanners(adsEngine.GetSearchBanners(), manager.lastQuery);
    [[MWMBannersCache cache] refreshWithCoreBanners:coreBanners];
  }
  [manager update];
}

+ (void)showResult:(search::Result const &)result { GetFramework().ShowSearchResult(result); }
+ (search::Result const &)resultWithContainerIndex:(NSUInteger)index
{
  return [MWMSearch manager]->m_everywhereResults[index];
}

+ (BOOL)isLocalAdsWithContainerIndex:(NSUInteger)index
{
  return [MWMSearch manager]->m_isLocalAdsCustomer[index];
}

+ (id<MWMBanner>)adWithContainerIndex:(NSUInteger)index
{
  return [[MWMSearch manager].banners bannerAtIndex:index];
}

+ (MWMSearchItemType)resultTypeWithRow:(NSUInteger)row
{
  auto itemsIndex = [MWMSearch manager].itemsIndex;
  return [itemsIndex resultTypeWithRow:row];
}

+ (NSUInteger)containerIndexWithRow:(NSUInteger)row
{
  auto itemsIndex = [MWMSearch manager].itemsIndex;
  return [itemsIndex resultContainerIndexWithRow:row];
}

+ (void)update { [[MWMSearch manager] update]; }

+ (void)reset
{
  auto manager = [MWMSearch manager];
  manager.lastSearchStamp++;
  GetFramework().CancelAllSearches();
  manager.everywhereSearchCompleted = NO;
  manager.viewportSearchCompleted = NO;
  if (manager->m_filterQuery != manager->m_everywhereParams.m_query)
    manager.isHotelResults = NO;
  [manager onSearchResultsUpdated];
}

+ (void)clear
{
  auto manager = [MWMSearch manager];
  manager->m_everywhereResults.Clear();
  manager->m_everywhereParams.m_query.clear();
  manager->m_viewportParams.m_query.clear();
  manager.suggestionsCount = 0;
  [manager.filter reset];
  [self reset];
}

+ (BOOL)isSearchOnMap { return [MWMSearch manager].searchOnMap; }

+ (void)setSearchOnMap:(BOOL)searchOnMap
{
  MWMSearch * manager = [MWMSearch manager];
  if (manager.searchOnMap == searchOnMap)
    return;
  manager.searchOnMap = searchOnMap;
}

+ (NSUInteger)suggestionsCount { return [MWMSearch manager].suggestionsCount; }
+ (NSUInteger)resultsCount { return [MWMSearch manager].itemsIndex.count; }
+ (BOOL)isHotelResults { return [MWMSearch manager].isHotelResults; }

#pragma mark - Filters

+ (BOOL)hasFilter { return [[MWMSearch manager].filter rules] != nullptr; }

+ (MWMSearchFilterViewController *)getFilter
{
  MWMSearch * manager = [MWMSearch manager];
  if (!manager.filter && manager.isHotelResults)
    manager.filter = [MWMSearchHotelsFilterViewController controller];
  return manager.filter;
}

+ (void)clearFilter
{
  MWMSearch * manager = [MWMSearch manager];
  [manager.filter reset];
  [manager update];
}

- (void)updateItemsIndexWithBannerReload:(BOOL)reloadBanner
{
  auto const resultsCount = self->m_everywhereResults.GetCount();
  auto const itemsIndex = [[MWMSearchIndex alloc] initWithSuggestionsCount:self.suggestionsCount
                                                              resultsCount:resultsCount];
  if (resultsCount > 0)
  {
    auto const & adsEngine = GetFramework().GetAdsEngine();
    if (![MWMSettings adForbidden] && adsEngine.HasSearchBanner())
    {
      self.banners = [[MWMSearchBanners alloc] initWithSearchIndex:itemsIndex];
      __weak auto weakSelf = self;
      [[MWMBannersCache cache]
          getWithCoreBanners:banner_helpers::MatchPriorityBanners(adsEngine.GetSearchBanners(), self.lastQuery)
                   cacheOnly:YES
                     loadNew:reloadBanner
                  completion:^(id<MWMBanner> ad, BOOL isAsync) {
                    __strong auto self = weakSelf;
                    if (!self)
                      return;
                    NSAssert(isAsync == NO, @"Banner is not from cache!");
                    [self.banners add:ad];
                  }];
    }
  }
  else
  {
    self.banners = nil;
  }
  [itemsIndex build];
  self.itemsIndex = itemsIndex;
}

#pragma mark - Notifications

- (void)onSearchStarted
{
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchStarted)])
      [observer onSearchStarted];
  }
}

- (void)onSearchCompleted
{
  [self updateItemsIndexWithBannerReload:YES];
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchCompleted)])
      [observer onSearchCompleted];
  }
}

- (void)onSearchResultsUpdated
{
  [self updateItemsIndexWithBannerReload:NO];
  for (Observer observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchResultsUpdated)])
      [observer onSearchResultsUpdated];
  }
}

#pragma mark - Properties

- (void)setSearchCount:(NSInteger)searchCount
{
  NSAssert((searchCount >= 0) &&
               ((_searchCount == searchCount - 1) || (_searchCount == searchCount + 1)),
           @"Invalid search count update");
  if (_searchCount == 0)
    [self onSearchStarted];
  else if (searchCount == 0)
    [self onSearchCompleted];
  _searchCount = searchCount;
}

@end
