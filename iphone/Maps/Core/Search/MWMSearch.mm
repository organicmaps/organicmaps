#import "MWMSearch.h"
#import <Crashlytics/Crashlytics.h>
#import "MWMBannerHelpers.h"
#import "MWMFrameworkListener.h"
#import "MWMSearchHotelsFilterViewController.h"
#import "MWMSearchManager+Filter.h"
#import "MWMSearchManager.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "partners_api/booking_availability_params.hpp"
#include "partners_api/ads_engine.hpp"

#include "map/everywhere_search_params.hpp"
#include "map/viewport_search_params.hpp"

#include "map/booking_filter_params.hpp"

#include "platform/network_policy.hpp"

#include <chrono>
#include <memory>
#include <utility>

namespace
{
using Observer = id<MWMSearchObserver>;
using Observers = NSHashTable<Observer>;

booking::filter::Tasks MakeBookingFilterTasks(booking::filter::Params && availabilityParams)
{
  booking::filter::Tasks tasks;
  if (availabilityParams.IsEmpty())
  {
    if (!platform::GetCurrentNetworkPolicy().CanUse())
      return {};
    
    auto params = GetFramework().GetLastBookingAvailabilityParams();
    if (params.IsEmpty())
      params = booking::AvailabilityParams::MakeDefault();
    params.m_dealsOnly = true;
    
    booking::filter::Params dp(std::make_shared<booking::AvailabilityParams>(params), {});
    tasks.EmplaceBack(booking::filter::Type::Deals, move(dp));
  }
  else
  {
    booking::AvailabilityParams dp;
    dp.Set(*(availabilityParams.m_apiParams));
    dp.m_dealsOnly = true;
    booking::filter::Params dealsParams(std::make_shared<booking::AvailabilityParams>(dp), {});
    
    tasks.EmplaceBack(booking::filter::Type::Availability, std::move(availabilityParams));
    tasks.EmplaceBack(booking::filter::Type::Deals, std::move(dealsParams));
  }
  
  return tasks;
}
}  // namespace

@interface MWMSearch ()<MWMFrameworkDrapeObserver>

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) BOOL searchOnMap;

@property(nonatomic) BOOL textChanged;

@property(nonatomic) Observers * observers;

@property(nonatomic) NSUInteger lastSearchTimestamp;

@property(nonatomic) MWMSearchFilterViewController * filter;

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
  search::Results m_viewportResults;
  std::unordered_map<booking::filter::Type, std::vector<FeatureID>> m_filterResults;
  std::vector<search::ProductInfo> m_productInfo;
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

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];
    [MWMFrameworkListener addObserver:self];
  }
  return self;
}

- (void)enableCallbackFor:(booking::filter::Type const)filterType {
  auto & tasks = self->m_everywhereParams.m_bookingFilterTasks;
  auto availabilityTaskIt = tasks.Find(filterType);
  if (availabilityTaskIt != tasks.end())
  {
    availabilityTaskIt->m_filterParams.m_callback =
    [self, filterType](shared_ptr<booking::ParamsBase> const & apiParams,
                        std::vector<FeatureID> const & sortedFeatures)
    {
      auto & t = self->m_everywhereParams.m_bookingFilterTasks;
      auto const it = t.Find(filterType);

      if (it == t.end())
        return;

      auto const & p = it->m_filterParams.m_apiParams;
      if (p->IsEmpty() || !p->Equals(*apiParams))
        return;

      self->m_filterResults[filterType] = sortedFeatures;
      [self onSearchResultsUpdated];
    };
  }
}

- (void)searchEverywhere
{
  self.lastSearchTimestamp += 1;
  NSUInteger const timestamp = self.lastSearchTimestamp;
  m_everywhereParams.m_onResults = [self, timestamp](
                                       search::Results const & results,
                                       std::vector<search::ProductInfo> const & productInfo) {

    if (timestamp == self.lastSearchTimestamp)
    {
      self->m_everywhereResults = results;
      self->m_productInfo = productInfo;
      self.suggestionsCount = results.GetSuggestsCount();

      [self onSearchResultsUpdated];
    }

    if (results.IsEndMarker())
      self.searchCount -= 1;
  };

  [self enableCallbackFor:booking::filter::Type::Availability];
  [self enableCallbackFor:booking::filter::Type::Deals];

  GetFramework().SearchEverywhere(m_everywhereParams);
  self.searchCount += 1;
}

- (void)searchInViewport
{
  m_viewportParams.m_onStarted = [self] { self.searchCount += 1; };
  m_viewportParams.m_onCompleted = [self](search::Results const & results) {
    if (!results.IsEndMarker())
      return;
    self->m_viewportResults = results;
    self.searchCount -= 1;
  };

  GetFramework().SearchInViewport(m_viewportParams);
}

- (void)updateFilters
{
  shared_ptr<search::hotels_filter::Rule> const hotelsRules = self.filter ? [self.filter rules] : nullptr;
  m_viewportParams.m_hotelsFilter = hotelsRules;
  m_everywhereParams.m_hotelsFilter = hotelsRules;
  
  auto availabilityParams =
    self.filter ? [self.filter availabilityParams] : booking::filter::Params();

  auto const tasks = MakeBookingFilterTasks(std::move(availabilityParams));

  m_viewportParams.m_bookingFilterTasks = tasks;
  m_everywhereParams.m_bookingFilterTasks = tasks;
}

- (void)update
{
  [self reset];
  if (m_everywhereParams.m_query.empty())
    return;
  [self updateFilters];

  if (IPAD)
  {
    [self searchInViewport];
    [self searchEverywhere];
  }
  else
  {
    if (self.searchOnMap)
      [self searchInViewport];
    else
      [self searchEverywhere];
  }
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
  auto const & purchase = GetFramework().GetPurchase();
  bool const hasSubscription = purchase && !purchase->IsSubscriptionActive(SubscriptionType::RemoveAds);
  
  if (hasSubscription && adsEngine.HasSearchBanner())
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

+ (search::ProductInfo const &)productInfoWithContainerIndex:(NSUInteger)index
{
  return [MWMSearch manager]->m_productInfo[index];
}

+ (id<MWMBanner>)adWithContainerIndex:(NSUInteger)index
{
  return [[MWMSearch manager].banners bannerAtIndex:index];
}

+ (BOOL)isFeatureAt:(NSUInteger)index in:(std::vector<FeatureID> const &)array
{
  auto const & result = [self resultWithContainerIndex:index];
  if (result.GetResultType() != search::Result::Type::Feature)
    return NO;
  auto const & resultFeatureID = result.GetFeatureID();
  return std::binary_search(array.begin(), array.end(), resultFeatureID);
}

+ (BOOL)isBookingAvailableWithContainerIndex:(NSUInteger)index
{
  return [self isFeatureAt:index in:[MWMSearch manager]->m_filterResults[booking::filter::Type::Availability]];
}

+ (BOOL)isDealAvailableWithContainerIndex:(NSUInteger)index
{
  return [self isFeatureAt:index in:[MWMSearch manager]->m_filterResults[booking::filter::Type::Deals]];
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

- (void)reset
{
  self.lastSearchTimestamp += 1;
  GetFramework().CancelAllSearches();

  m_everywhereResults.Clear();
  m_viewportResults.Clear();

  m_filterResults.clear();
  m_viewportParams.m_bookingFilterTasks.Clear();
  m_everywhereParams.m_bookingFilterTasks.Clear();

  [self onSearchResultsUpdated];
}

+ (void)clear
{
  auto manager = [MWMSearch manager];
  manager->m_everywhereParams.m_query.clear();
  manager->m_viewportParams.m_query.clear();
  manager.suggestionsCount = 0;
  manager.filter = nil;
  [manager reset];
}

+ (void)setSearchOnMap:(BOOL)searchOnMap
{
  if (IPAD)
    return;
  MWMSearch * manager = [MWMSearch manager];
  if (manager.searchOnMap == searchOnMap)
    return;
  manager.searchOnMap = searchOnMap;
  if (searchOnMap && ![MWMRouter isRoutingActive])
    GetFramework().ShowSearchResults(manager->m_everywhereResults);
  [manager update];
}

+ (NSUInteger)suggestionsCount { return [MWMSearch manager].suggestionsCount; }
+ (NSUInteger)resultsCount { return [MWMSearch manager].itemsIndex.count; }
+ (BOOL)isHotelResults { return [[MWMSearch manager] isHotelResults]; }

#pragma mark - Filters

+ (BOOL)hasFilter
{
  auto filter = [MWMSearch manager].filter;
  if (!filter)
    return NO;
  auto const hasRules = [filter rules] != nullptr;
  auto const hasBookingParams = ![filter availabilityParams].IsEmpty();
  return hasRules || hasBookingParams;
}

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
  manager.filter = nil;
  [manager update];
}

+ (void)showHotelFilterWithParams:(search_filter::HotelParams &&)params
                 onFinishCallback:(MWMVoidBlock)callback
{
  auto filter =
  static_cast<MWMSearchHotelsFilterViewController *>([MWMSearchHotelsFilterViewController controller]);
  auto search = [MWMSearch manager];
  search.filter = filter;
  [[MWMSearchManager manager] updateFilter:[filter, callback, params = std::move(params)]() mutable
  {
    [filter applyParams:std::move(params) onFinishCallback:callback];
  }];
}

- (void)updateItemsIndexWithBannerReload:(BOOL)reloadBanner
{
  auto const resultsCount = self->m_everywhereResults.GetCount();
  auto const itemsIndex = [[MWMSearchIndex alloc] initWithSuggestionsCount:self.suggestionsCount
                                                              resultsCount:resultsCount];
  if (resultsCount > 0)
  {
    auto const & adsEngine = GetFramework().GetAdsEngine();
    auto const & purchase = GetFramework().GetPurchase();
    bool const hasSubscription = purchase && !purchase->IsSubscriptionActive(SubscriptionType::RemoveAds);

    if (hasSubscription && adsEngine.HasSearchBanner())
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

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportChangedEvent
{
  if (!GetFramework().GetSearchAPI().IsViewportSearchActive())
    return;
  if (IPAD)
    [self searchEverywhere];
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

- (BOOL)isHotelResults
{
  return m_everywhereResults.GetType() == search::Results::Type::Hotels ||
         m_viewportResults.GetType() == search::Results::Type::Hotels;
}

@end
