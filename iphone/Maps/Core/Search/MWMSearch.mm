#import "MWMSearch.h"
#import "MWMFrameworkListener.h"
#import "MWMFrameworkObservers.h"
#import "SearchResult+Core.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>
#include <CoreApi/MWMTypes.h>

#include "platform/network_policy.hpp"

namespace
{
using Observer = id<MWMSearchObserver>;
using Observers = NSHashTable<Observer>;
}  // namespace

@interface MWMSearch () <MWMFrameworkDrapeObserver>

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) SearchMode searchMode;
@property(nonatomic) BOOL textChanged;
@property(nonatomic) Observers * observers;
@property(nonatomic) NSUInteger lastSearchTimestamp;
@property(nonatomic) SearchIndex * itemsIndex;
@property(nonatomic) NSInteger searchCount;

@end

@implementation MWMSearch
{
  std::string m_query;
  std::string m_locale;
  bool m_isCategory;
  search::Results m_everywhereResults;
  search::Results m_viewportResults;
}

#pragma mark - Instance

+ (MWMSearch *)manager
{
  static MWMSearch * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ manager = [[self alloc] initManager]; });
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

- (void)searchEverywhere
{
  self.lastSearchTimestamp += 1;
  NSUInteger const timestamp = self.lastSearchTimestamp;

  search::EverywhereSearchParams params{
      m_query,
      m_locale,
      {} /* default timeout */,
      m_isCategory,
      // m_onResults
      [self, timestamp](search::Results results, std::vector<search::ProductInfo> productInfo)
  {
    // Store the flag first, because we will make move next.
    bool const isEndMarker = results.IsEndMarker();

    if (timestamp == self.lastSearchTimestamp)
    {
      self.suggestionsCount = results.GetSuggestsCount();
      self->m_everywhereResults = std::move(results);

      [self onSearchResultsUpdated];
    }

    if (isEndMarker)
      self.searchCount -= 1;
  }};

  GetFramework().GetSearchAPI().SearchEverywhere(std::move(params));
  self.searchCount += 1;
}

- (void)searchInViewport
{
  search::ViewportSearchParams params{m_query,
                                      m_locale,
                                      {} /* default timeout */,
                                      m_isCategory,
                                      // m_onStarted
                                      {},
                                      // m_onCompleted
                                      [self](search::Results results)
  {
    if (!results.IsEndMarker())
      return;
    if (!results.IsEndedCancelled())
      self->m_viewportResults = std::move(results);
  }};

  GetFramework().GetSearchAPI().SearchInViewport(std::move(params));
}

- (void)update
{
  if (m_query.empty())
    return;

  switch (self.searchMode)
  {
  case SearchModeEverywhere: [self searchEverywhere]; break;
  case SearchModeViewport: [self searchInViewport]; break;
  case SearchModeEverywhereAndViewport:
    [self searchEverywhere];
    [self searchInViewport];
    break;
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

+ (void)saveQuery:(SearchQuery *)query
{
  if (!query.text || query.text.length == 0)
    return;

  std::string locale = (!query.locale || query.locale == 0) ? [MWMSearch manager]->m_locale : query.locale.UTF8String;
  std::string text = query.text.UTF8String;
  GetFramework().GetSearchAPI().SaveSearchQuery({std::move(locale), std::move(text)});
}

+ (void)searchQuery:(SearchQuery *)query
{
  if (!query.text)
    return;

  MWMSearch * manager = [MWMSearch manager];
  if (query.locale.length != 0)
    manager->m_locale = query.locale.UTF8String;

  // Pass input query as-is without any normalization (precomposedStringWithCompatibilityMapping).
  // Otherwise № -> No, and it's unexpectable for the search index.
  manager->m_query = query.text.UTF8String;
  manager->m_isCategory = (query.source == SearchTextSourceCategory);
  manager.textChanged = YES;

  [manager reset];
  [manager update];
}

+ (void)showResultAtIndex:(NSUInteger)index
{
  auto const & result = [MWMSearch manager]->m_everywhereResults[index];
  GetFramework().StopLocationFollow();
  GetFramework().SelectSearchResult(result, true);
}

+ (SearchResult *)resultWithContainerIndex:(NSUInteger)index
{
  SearchResult * result = [[SearchResult alloc] initWithResult:[MWMSearch manager]->m_everywhereResults[index]
                                                      itemType:[MWMSearch resultTypeWithRow:index]
                                                         index:index];
  return result;
}

+ (NSArray<SearchResult *> *)getResults
{
  NSMutableArray<SearchResult *> * results = [[NSMutableArray alloc] initWithCapacity:MWMSearch.resultsCount];
  for (NSUInteger i = 0; i < MWMSearch.resultsCount; ++i)
  {
    SearchResult * result = [MWMSearch resultWithContainerIndex:i];
    [results addObject:result];
  }
  return [results copy];
}

+ (SearchItemType)resultTypeWithRow:(NSUInteger)row
{
  auto itemsIndex = [MWMSearch manager].itemsIndex;
  return [itemsIndex resultTypeWithRow:row];
}

+ (NSUInteger)containerIndexWithRow:(NSUInteger)row
{
  auto itemsIndex = [MWMSearch manager].itemsIndex;
  return [itemsIndex resultContainerIndexWithRow:row];
}

- (void)reset
{
  self.lastSearchTimestamp += 1;
  GetFramework().GetSearchAPI().CancelAllSearches();

  m_everywhereResults.Clear();
  m_viewportResults.Clear();

  [self onSearchResultsUpdated];
}

+ (void)clear
{
  auto manager = [MWMSearch manager];
  manager->m_query.clear();
  manager.suggestionsCount = 0;
  [manager reset];
}

+ (SearchMode)searchMode
{
  return [MWMSearch manager].searchMode;
}

+ (void)setSearchMode:(SearchMode)mode
{
  MWMSearch * manager = [MWMSearch manager];
  if (manager.searchMode == mode)
    return;
  manager.searchMode = mode;
  [manager update];
}

+ (NSUInteger)suggestionsCount
{
  return [MWMSearch manager].suggestionsCount;
}

+ (NSUInteger)resultsCount
{
  return [MWMSearch manager].itemsIndex.count;
}

- (void)updateItemsIndexWithBannerReload:(BOOL)reloadBanner
{
  auto const resultsCount = self->m_everywhereResults.GetCount();
  auto const itemsIndex = [[SearchIndex alloc] initWithSuggestionsCount:self.suggestionsCount
                                                           resultsCount:resultsCount];
  [itemsIndex build];
  self.itemsIndex = itemsIndex;
}

#pragma mark - Notifications

- (void)onSearchStarted
{
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onSearchStarted)])
      [observer onSearchStarted];
}

- (void)onSearchCompleted
{
  [self updateItemsIndexWithBannerReload:YES];
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onSearchCompleted)])
      [observer onSearchCompleted];
}

- (void)onSearchResultsUpdated
{
  [self updateItemsIndexWithBannerReload:NO];
  for (Observer observer in self.observers)
    if ([observer respondsToSelector:@selector(onSearchResultsUpdated)])
      [observer onSearchResultsUpdated];
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)processViewportChangedEvent
{
  if (!GetFramework().GetSearchAPI().IsViewportSearchActive())
    return;

  BOOL const isSearchCompleted = self.searchCount == 0;
  if (!isSearchCompleted)
    return;

  switch (self.searchMode)
  {
  case SearchModeEverywhere:
  case SearchModeViewport: break;
  case SearchModeEverywhereAndViewport: [self searchEverywhere]; break;
  }
}

#pragma mark - Properties

- (void)setSearchCount:(NSInteger)searchCount
{
  NSAssert((searchCount >= 0) && ((_searchCount == searchCount - 1) || (_searchCount == searchCount + 1)),
           @"Invalid search count update");
  if (searchCount > 0)
    [self onSearchStarted];
  else if (searchCount == 0)
    [self onSearchCompleted];
  _searchCount = searchCount;
}

@end
