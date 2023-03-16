#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, MWMNetworkPolicyPermission) {
  MWMNetworkPolicyPermissionAsk,
  MWMNetworkPolicyPermissionAlways,
  MWMNetworkPolicyPermissionNever,
  MWMNetworkPolicyPermissionToday,
  MWMNetworkPolicyPermissionNotToday
};

typedef NS_ENUM(NSInteger, MWMConnectionType) {
  MWMConnectionTypeNone,
  MWMConnectionTypeWifi,
  MWMConnectionTypeCellular
} NS_SWIFT_NAME(ConnectionType);

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(NetworkPolicy)
@interface MWMNetworkPolicy: NSObject

@property(nonatomic) MWMNetworkPolicyPermission permission;
@property(nonatomic, readonly) NSDate *permissionExpirationDate;
@property(nonatomic, readonly) BOOL canUseNetwork;
@property(nonatomic, readonly) MWMConnectionType connectionType;

+ (MWMNetworkPolicy *)sharedPolicy;

@end

NS_ASSUME_NONNULL_END
