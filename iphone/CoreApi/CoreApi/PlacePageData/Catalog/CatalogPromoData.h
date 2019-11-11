#import <Foundation/Foundation.h>

@class CatalogPromoItem;

NS_ASSUME_NONNULL_BEGIN

@interface CatalogPromoData : NSObject

@property(nonatomic, readonly) NSArray<CatalogPromoItem *> *promoItems;
@property(nonatomic, readonly, nullable) NSURL *moreUrl;
@property(nonatomic, readonly) NSString *tagsString;

@end

NS_ASSUME_NONNULL_END
