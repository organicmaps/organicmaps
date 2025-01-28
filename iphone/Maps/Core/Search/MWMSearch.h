#import "MWMSearchObserver.h"
#import "SearchItemType.h"

NS_ASSUME_NONNULL_BEGIN

@class SearchResult;

@protocol SearchManager
+ (void)addObserver:(id<MWMSearchObserver>)observer;
+ (void)removeObserver:(id<MWMSearchObserver>)observer;

+ (void)saveQuery:(NSString *)query forInputLocale:(NSString *)inputLocale;
+ (void)searchQuery:(NSString *)query forInputLocale:(NSString *)inputLocale withCategory:(BOOL)isCategory;

+ (void)showResultAtIndex:(NSUInteger)index;
+ (void)showEverywhereSearchResultsOnMap;
+ (void)showViewportSearchResultsOnMap;

+ (NSArray<SearchResult *> *)getResults;

+ (void)clear;
@end

NS_SWIFT_NAME(Search)
@interface MWMSearch : NSObject<SearchManager>

+ (SearchItemType)resultTypeWithRow:(NSUInteger)row;
+ (NSUInteger)containerIndexWithRow:(NSUInteger)row;
+ (SearchResult *)resultWithContainerIndex:(NSUInteger)index;

+ (NSUInteger)suggestionsCount;
+ (NSUInteger)resultsCount;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)new __attribute__((unavailable("call +manager instead")));

@end

NS_ASSUME_NONNULL_END
