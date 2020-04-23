#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface GuidesGalleryItem : NSObject

@property(nonatomic, readonly) NSString *guideId;
@property(nonatomic, readonly) NSString *url;
@property(nonatomic, readonly) NSString *imageUrl;
@property(nonatomic, readonly) NSString *title;
@property(nonatomic, readonly) BOOL downloaded;

@end

@interface CityGalleryItem : GuidesGalleryItem

@property(nonatomic, readonly) NSInteger bookmarksCount;
@property(nonatomic, readonly) BOOL hasTrack;

@end

@interface OutdoorGalleryItem : GuidesGalleryItem

@property(nonatomic, readonly) NSString *tag;
@property(nonatomic, readonly) double distance;
@property(nonatomic, readonly) NSUInteger duration;
@property(nonatomic, readonly) NSUInteger ascent;

@end

NS_ASSUME_NONNULL_END
