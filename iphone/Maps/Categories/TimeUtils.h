
#import <Foundation/Foundation.h>

@interface NSDateFormatter (Utils)

+ (NSString *)estimatedArrivalTimeWithSeconds:(NSNumber *)seconds;
+ (NSDate *)dateWithString:(NSString *)dateString;

@end