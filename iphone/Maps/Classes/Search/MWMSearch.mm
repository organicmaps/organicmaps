#import "MWMSearch.h"
#import <Crashlytics/Crashlytics.h>
#import "Common.h"
#import "MWMLocationManager.h"

#include "Framework.h"

#include "search/query_saver.hpp"
#include "search/search_params.hpp"

namespace
{
using TObserver = id<MWMSearchObserver>;
using TObservers = NSHashTable<__kindof TObserver>;
}  // namespace

@interface MWMSearch ()

@property(nonatomic) NSUInteger suggestionsCount;
@property(nonatomic) NSUInteger resultsCount;
@property(nonatomic) BOOL searchOnMap;

@property(nonatomic) BOOL textChanged;
@property(nonatomic) BOOL isSearching;
@property(nonatomic) BOOL isPendingUpdate;

@property(nonatomic) TObservers * observers;

@end

@implementation MWMSearch
{
  search::SearchParams m_params;
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
  {
    _observers = [TObservers weakObjectsHashTable];
    __weak auto weakSelf = self;
    m_params.m_onStarted = [weakSelf] {
      __strong auto self = weakSelf;
      if (!self)
        return;
      runAsyncOnMainQueue(^{
        [self onSearchStarted];
      });
    };
    m_params.m_onResults = [weakSelf](search::Results const & results) {
      __strong auto self = weakSelf;
      if (!self)
        return;
      runAsyncOnMainQueue([self, results] {
        if (results.IsEndMarker())
        {
          [self onSearchCompleted];
        }
        else
        {
          self->m_results = results;
          self.suggestionsCount = results.GetSuggestsCount();
          self.resultsCount = results.GetCount();
          [self onSearchResultsUpdated];
        }
      });
    };
  }
  return self;
}

- (void)update
{
  if (self.isSearching)
  {
    self.isPendingUpdate = YES;
    return;
  }
  self.isPendingUpdate = NO;
  if (m_params.m_query.empty())
    return;
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  if (lastLocation)
    m_params.SetPosition(lastLocation.coordinate.latitude, lastLocation.coordinate.longitude);
  if (self.searchOnMap)
    GetFramework().StartInteractiveSearch(m_params);
  else
    GetFramework().Search(m_params);
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
                            ? [MWMSearch manager]->m_params.m_inputLocale
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
  if (inputLocale && inputLocale.length != 0)
  {
    string const locale = inputLocale.UTF8String;
    manager->m_params.SetInputLocale(locale);
  }
  string const text = query.precomposedStringWithCompatibilityMapping.UTF8String;
  manager->m_params.m_query = text;
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
  GetFramework().CancelInteractiveSearch();
  MWMSearch * manager = [MWMSearch manager];
  manager->m_params.Clear();
  manager->m_results.Clear();
  manager.suggestionsCount = manager->m_results.GetSuggestsCount();
  manager.resultsCount = manager->m_results.GetCount();
}

+ (BOOL)isSearchOnMap { return IPAD || [MWMSearch manager].searchOnMap; }
+ (void)setSearchOnMap:(BOOL)searchOnMap
{
  MWMSearch * manager = [MWMSearch manager];
  manager.searchOnMap = searchOnMap;
  [manager update];
}

+ (NSUInteger)suggestionsCount { return [MWMSearch manager].suggestionsCount; }
+ (NSUInteger)resultsCount { return [MWMSearch manager].resultsCount; }

#pragma mark - Notifications

- (void)onSearchStarted
{
  self.isSearching = YES;
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchStarted)])
      [observer onSearchStarted];
  }
}

- (void)onSearchCompleted
{
  self.isSearching = NO;
  if (self.isPendingUpdate)
    [self update];
  for (TObserver observer in self.observers)
  {
    if ([observer respondsToSelector:@selector(onSearchCompleted)])
      [observer onSearchCompleted];
  }
  // TODO(igrechuhin): Remove this workaround on search interface refactoring.
  if (IPAD)
    GetFramework().ShowSearchResults(m_results);
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
