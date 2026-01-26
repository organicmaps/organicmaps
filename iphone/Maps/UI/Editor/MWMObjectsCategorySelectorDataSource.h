@interface MWMObjectsCategorySelectorDataSource : NSObject

- (void)search:(NSString *)query;
- (NSString *)getTranslation:(NSInteger)row;
- (NSString *)getRecentCategoriesTranslation:(NSInteger)row;
- (NSString *)getType:(NSInteger)row;
- (NSString *)getRecentCategoriesType:(NSInteger)row;
- (NSInteger)size;
- (NSInteger)recentCategoriesListSize;
- (void)addToRecentCategories:(NSString *)query;
@end
