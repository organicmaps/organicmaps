#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void(^GalleryChangedBlock)(bool reloadGallery);

NS_SWIFT_NAME(GuidesManager)
@interface MWMGuidesManager : NSObject

+ (instancetype)sharedManager;
- (NSString *)activeGuideId;
- (void)setActiveGuide:(NSString *)guideId;
- (void)setGalleryChangedCallback:(GalleryChangedBlock)callback;
- (void)resetGalleryChangedCallback;

@end

NS_ASSUME_NONNULL_END
