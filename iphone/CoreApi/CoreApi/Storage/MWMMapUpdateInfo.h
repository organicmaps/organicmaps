#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(MapUpdateInfo)
@interface MWMMapUpdateInfo : NSObject

@property(nonatomic, readonly) uint32_t numberOfFiles;
@property(nonatomic, readonly) uint64_t updateSize;
/// Net change in disk usage, in bytes.
/// Net change in disk usage, in bytes; negative when the update frees space.
@property(nonatomic, readonly) int64_t differenceSize;

@end

NS_ASSUME_NONNULL_END
