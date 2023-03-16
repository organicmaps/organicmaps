NS_ASSUME_NONNULL_BEGIN
@class MWMCarPlaySearchResultObject;

API_AVAILABLE(ios(12.0))
NS_SWIFT_NAME(CarPlaySearchService)
@interface MWMCarPlaySearchService : NSObject
@property(strong, nonatomic, readonly) NSArray<MWMCarPlaySearchResultObject *> *lastResults;

- (instancetype)init;

- (void)searchText:(NSString *)text
    forInputLocale:(NSString *)inputLocale
 completionHandler:(void (^)(NSArray<MWMCarPlaySearchResultObject *> *searchResults))completionHandler;
- (void)saveLastQuery;
@end

NS_ASSUME_NONNULL_END
