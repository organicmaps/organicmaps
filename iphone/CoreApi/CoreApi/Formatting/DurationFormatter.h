#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface DurationFormatter : NSObject

+ (NSString *)routingDurationStringFromTimeInterval:(NSTimeInterval)timeInterval
    NS_SWIFT_NAME(routingDurationString(from:));

@end

NS_ASSUME_NONNULL_END
