#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface SwizzleStyle : NSObject

+ (void)swizzle:(Class)forClass methodName:(NSString*)methodName;

@end

NS_ASSUME_NONNULL_END
