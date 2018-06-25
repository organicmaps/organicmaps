@interface MWMCatalogCategory : NSObject

@property (nonatomic) MWMMarkGroupID categoryId;
@property (copy, nonatomic) NSString * title;
@property (copy, nonatomic) NSString * author;
@property (copy, nonatomic) NSString * annotation;
@property (copy, nonatomic) NSString * detailedAnnotation;
@property (nonatomic) NSInteger bookmarksCount;
@property (nonatomic) BOOL visible;

@end
