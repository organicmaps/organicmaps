#import <CoreApi/MWMTypes.h>

#import "MWMSearchItemType.h"
#import "MWMSearchObserver.h"

namespace search {
class Result;
struct ProductInfo;
}  // namespace search

@interface MWMSearch : NSObject

+ (void)addObserver:(id<MWMSearchObserver>)observer;
+ (void)removeObserver:(id<MWMSearchObserver>)observer;

+ (void)saveQuery:(NSString *)query forInputLocale:(NSString *)inputLocale;
+ (void)searchQuery:(NSString *)query forInputLocale:(NSString *)inputLocale withCategory:(BOOL)isCategory;

+ (void)showResult:(search::Result const &)result;

+ (MWMSearchItemType)resultTypeWithRow:(NSUInteger)row;
+ (NSUInteger)containerIndexWithRow:(NSUInteger)row;
+ (search::Result const &)resultWithContainerIndex:(NSUInteger)index;
+ (search::ProductInfo const &)productInfoWithContainerIndex:(NSUInteger)index;

+ (void)clear;

+ (void)setSearchOnMap:(BOOL)searchOnMap;

+ (NSUInteger)suggestionsCount;
+ (NSUInteger)resultsCount;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)new __attribute__((unavailable("call +manager instead")));

@end
