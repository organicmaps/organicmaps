#import <CoreApi/MWMNetworkPolicy.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMNetworkPolicy (UI)

- (void)callOnlineApi:(MWMBoolBlock)onlineCall;
- (void)callOnlineApi:(MWMBoolBlock)onlineCall forceAskPermission:(BOOL)askPermission;

@end

NS_ASSUME_NONNULL_END
