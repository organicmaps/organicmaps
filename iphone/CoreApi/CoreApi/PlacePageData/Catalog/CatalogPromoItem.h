#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CatalogPromoItem : NSObject

@property(nonatomic, readonly) NSString *placeTitle;
@property(nonatomic, readonly) NSString *placeDescription;
@property(nonatomic, readonly) NSString *imageUrl;
@property(nonatomic, readonly) NSString *catalogUrl;
@property(nonatomic, readonly) NSString *guideName;
@property(nonatomic, readonly) NSString *guideAuthor;
@property(nonatomic, readonly) NSString *categoryLabel;
@property(nonatomic, readonly) NSString *hexColor;

@end

NS_ASSUME_NONNULL_END
