#import "MWMSearchObserver.h"
#import "SearchItemType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, SearchTextSource) {
  SearchTextSourceTypedText,
  SearchTextSourceCategory,
  SearchTextSourceHistory,
  SearchTextSourceSuggestion,
  SearchTextSourceDeeplink
};

typedef NS_ENUM(NSUInteger, SearchMode) {
  SearchModeEverywhere,
  SearchModeViewport,
  SearchModeEverywhereAndViewport
};

@class SearchResult;
@class SearchQuery;

@protocol SearchManager

+ (void)addObserver:(id<MWMSearchObserver>)observer;
+ (void)removeObserver:(id<MWMSearchObserver>)observer;

+ (void)saveQuery:(SearchQuery *)query;
+ (void)searchQuery:(SearchQuery *)query;

+ (void)showResultAtIndex:(NSUInteger)index;
+ (SearchMode)searchMode;
+ (void)setSearchMode:(SearchMode)mode;

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
