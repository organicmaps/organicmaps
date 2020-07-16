#import <CoreApi/CoreBanner.h>
#import <CoreApi/MWMTypes.h>

#import "MWMHotelParams.h"
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
+ (void)searchQuery:(NSString *)query forInputLocale:(NSString *)inputLocale;

+ (void)showResult:(search::Result const &)result;

+ (MWMSearchItemType)resultTypeWithRow:(NSUInteger)row;
+ (NSUInteger)containerIndexWithRow:(NSUInteger)row;
+ (search::Result const &)resultWithContainerIndex:(NSUInteger)index;
+ (search::ProductInfo const &)productInfoWithContainerIndex:(NSUInteger)index;
+ (id<MWMBanner>)adWithContainerIndex:(NSUInteger)index;
+ (BOOL)isBookingAvailableWithContainerIndex:(NSUInteger)index;
+ (BOOL)isDealAvailableWithContainerIndex:(NSUInteger)index;

+ (void)updateHotelFilterWithParams:(MWMHotelParams *)params;
+ (void)clear;

+ (void)setSearchOnMap:(BOOL)searchOnMap;

+ (NSUInteger)suggestionsCount;
+ (NSUInteger)resultsCount;

+ (BOOL)isHotelResults;

+ (BOOL)hasFilter;
+ (BOOL)hasAvailability;
+ (int)filterCount;
+ (MWMHotelParams *)getFilter;
+ (void)clearFilter;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)new __attribute__((unavailable("call +manager instead")));

@end
