#import <Foundation/Foundation.h>

@class GuidesGalleryItem;

NS_ASSUME_NONNULL_BEGIN

@interface GuidesGalleryData : NSObject

@property(nonatomic, readonly) NSArray<GuidesGalleryItem *> *galleryItems;
@property(nonatomic, readonly) NSString *activeGuideId;

@end

NS_ASSUME_NONNULL_END
