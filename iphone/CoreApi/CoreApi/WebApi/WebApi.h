#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface WebApi : NSObject

+ (NSDictionary<NSString *, NSString *> *)getDefaultAuthHeaders;

@end

NS_ASSUME_NONNULL_END
