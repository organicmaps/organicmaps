#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMSearchFrameworkHelper : NSObject

+ (NSArray<NSString *> *)searchCategories;

+ (BOOL)isSearchHistoryEmpty;
+ (BOOL)isLanguageSupported:(NSString *)languageCode;
+ (NSArray<NSString *> *)lastSearchQueries;
+ (void)clearSearchHistory;

@end

NS_ASSUME_NONNULL_END
