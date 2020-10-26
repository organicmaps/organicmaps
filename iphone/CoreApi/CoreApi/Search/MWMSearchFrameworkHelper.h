#import <Foundation/Foundation.h>

@protocol MWMBanner;

NS_ASSUME_NONNULL_BEGIN

@interface MWMSearchFrameworkHelper : NSObject

- (NSArray<NSString *> *)searchCategories;
- (nullable id<MWMBanner>)searchCategoryBanner;

- (BOOL)isSearchHistoryEmpty;
- (NSArray<NSString *> *)lastSearchQueries;
- (void)clearSearchHistory;

@end

NS_ASSUME_NONNULL_END
