#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TrackRecordingInfo : NSObject

@property (nonatomic, readonly) NSString * distance;
@property (nonatomic, readonly) NSString * duration;
@property (nonatomic, readonly) NSString * ascent;
@property (nonatomic, readonly) NSString * descent;
@property (nonatomic, readonly) NSString * maxElevation;
@property (nonatomic, readonly) NSString * minElevation;

@end

NS_ASSUME_NONNULL_END
