#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ElevationHeightPoint : NSObject

@property(nonatomic, readonly) double distance;
@property(nonatomic, readonly) double height;

- (instancetype)initWithDistance:(double)distance andHeight:(double)height;

@end

NS_ASSUME_NONNULL_END
