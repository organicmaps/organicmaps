#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(RouteSpeedSettings)
@interface MWMRouteSpeedSettings : NSObject

@property(nonatomic) double cruisingSpeedKMpH;
@property(nonatomic) NSInteger windSpeedMpS;
@property(nonatomic) NSInteger windDirectionDegrees;
@property(nonatomic, readonly) double minimumSpeedKMpH;
@property(nonatomic, readonly) double maximumSpeedKMpH;
@property(nonatomic, readonly) double speedStepKMpH;
@property(nonatomic, readonly) double defaultSpeedKMpH;
@property(nonatomic, readonly) NSInteger maximumWindSpeedMpS;
@property(nonatomic, readonly) BOOL windSupported;
@property(nonatomic, readonly) NSInteger changedCount;

@property(class, nonatomic, readonly) NSInteger defaultWindSpeedMpS;
@property(class, nonatomic, readonly) NSInteger windDirectionStepDegrees;

+ (nullable instancetype)current;

- (void)save;

@end

NS_ASSUME_NONNULL_END
