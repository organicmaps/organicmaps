#import "MWMMyPositionMode.h"
NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(LocationModeListener)
@protocol MWMLocationModeListener <NSObject>
- (void)processMyPositionStateModeEvent:(MWMMyPositionMode)mode;
- (void)processMyPositionPendingTimeout;
@end

NS_ASSUME_NONNULL_END

