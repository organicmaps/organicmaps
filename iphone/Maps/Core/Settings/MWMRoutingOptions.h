#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(RoutingOptions)
@interface MWMRoutingOptions : NSObject

@property(nonatomic) BOOL avoidToll;
@property(nonatomic) BOOL avoidDirty;
@property(nonatomic) BOOL avoidFerry;
@property(nonatomic) BOOL avoidMotorway;
@property(nonatomic, readonly) BOOL hasOptions;

- (void)save;

@end

NS_ASSUME_NONNULL_END
