#import "MWMSearch.h"
#import <Crashlytics/Crashlytics.h>
#import "Common.h"
#import "MWMLocationManager.h"

#include "Framework.h"

#include "search/everywhere_search_params.hpp"
#include "search/query_saver.hpp"
#include "search/viewport_search_params.hpp"

namespace
{
using TObserver = id<MWMSearchObserver>;
using TObservers = NSHashTable<__kindof TObserver>;

NSTimeInterval constexpr kOnSearchCompletedDelay = 0.2;
}  // namespace

@interface MWMSearch ()

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) BOOL searchOnMap;

@property(nonatomic) BOOL textChanged;
@property(nonatomic) BOOL everywhereSearchActive;
@property(nonatomic) BOOL viewportSearchActive;

@property(nonatomic) TObservers * observers;

@end

@implementation MWMSearch
{
  search::EverywhereSearchParams m_everywhereParams;
  search::ViewportSearchParams m_viewportParams;
  search::Results m_results;
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
    _observers = [TObservers weakObjectsHashTable];
  return self;
}

- (void)updateCallbacks
{
  __weak auto weakSelf = self;
  m_everywhereParams.m_onResults = [weakSelf](search::Results const & results) {
    __strong auto self = weakSelf;
    if (!self)
      return;
    if (results.IsEndMarker())
    {
      self.everywhereSearchActive = NO;
      [self delayedOnSearchCompleted];
    }
    else
    {
      self->m_results = results;
      self.suggestionsCount = results.GetSuggestsCount();
      [self onSearchResultsUpdated];
    }
  };
  m_viewportParams.m_onStarted = [weakSelf] {
    __strong auto self = weakSelf;
    if (!self)
      return;
    if (IPAD)
    {
      GetFramework().SearchEverywhere(self->m_everywhereParams);
      self.everywhereSearchActive = YES;
    }
    self.viewportSearchActive = YES;
    [self onSearchStarted];
  };
  m_viewportParams.m_onCompleted = [weakSelf] {
    __strong auto self = weakSelf;
    if (!self)
      return;
    self.viewportSearchActive = NO;
    [self delayedOnSearchCompleted];
  };
}

- (void)update
{
  if (m_everywhereParams.m_query.empty())
    return;
  [self updateCallbacks];
  auto & f = GetFramework();
  if (IPAD)
  {
    f.SearchEverywhere(m_everywhereParams);
    f.SearchInViewport(m_viewportParams);

    self.everywhereSearchActive = YES;
  }
  else
  {
    if (self.searchOnMap)
    {
      f.SearchInViewport(m_viewportParams);
    }
    else
    {
      f.SearchEverywhere(m_everywhereParams);
      self.everywhereSearchActive = YES;
    }
  }
  [self onSearchStarted];
}

- (void)delayedOnSearchCompleted
{
  SEL const selector = @selector(onSearchCompleted);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:selector object:nil];
  [self performSelector:selector withObject:nil afterDelay:kOnSearchCompletedDelay];
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
  string const text = query.precomposedStringWithCompatibilityMapping.UTF8String;
  manager->m_everywhereParams.m_query = text;
  manager->m_viewportParams.m_query = text;
  manager.textChanged = YES;
  [manager update];
}

+ (void)showResult:(search::Result const &)result { GetFramework().ShowSearchResult(result); }
+ (search::Result &)resultAtIndex:(NSUInteger)index
{
  return [MWMSearch manager]->m_results.GetResult(index);
}

+ (void)clear
{
  GetFramework().CancelAllSearches();
  MWMSearch * manager = [MWMSearch manager];
  manager->m_results.Clear();
  manager.suggestionsCount = 0;
}

+ (BOOL)isSearchOnMap { return [MWMSearch manager].searchOnMap; }
+ (void)setSearchOnMap:(BOOL)searchOnMap
{
  MWMSearch * manager = [MWMSearch manager];
  manager.searchOnMap = searchOnMap;
  if (!IPAD)
    [manager update];
}

+ (NSUInteger)suggestionsCount { return [MWMSearch manager].suggestionsCount; }
+ (NSUInteger)resultsCount { return [MWMSearch manager]->m_results.GetCount(); }
#pragma mark - Notifications

- (void)onSearchStarted
{
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(onSearchCompleted) object:nil];

  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchStarted)])
      [observer onSearchStarted];
  }
}

- (void)onSearchCompleted
{
  if (self.everywhereSearchActive || self.viewportSearchActive)
    return;

  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchCompleted)])
      [observer onSearchCompleted];
  }
}

- (void)onSearchResultsUpdated
{
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchResultsUpdated)])
      [observer onSearchResultsUpdated];
  }
}

@end
