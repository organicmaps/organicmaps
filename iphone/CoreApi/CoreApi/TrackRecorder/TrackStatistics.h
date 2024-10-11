#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TrackStatistics : NSObject

@property (nonatomic, readonly) double length;
@property (nonatomic, readonly) double duration;
@property (nonatomic, readonly) double elevationGain;

@end

NS_ASSUME_NONNULL_END
