#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ElevationHeightPoint : NSObject

@property(nonatomic, readonly) double distance;
@property(nonatomic, readonly) double altitude;

- (instancetype)initWithDistance:(double)distance andAltitude:(double)altitude;

@end

NS_ASSUME_NONNULL_END
