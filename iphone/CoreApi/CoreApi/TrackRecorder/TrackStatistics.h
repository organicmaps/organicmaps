#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TrackStatistics : NSObject

@property (nonatomic, readonly) double length;
@property (nonatomic, readonly) double duration;
@property (nonatomic, readonly) double ascend;
@property (nonatomic, readonly) double descend;

@end

NS_ASSUME_NONNULL_END
