#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface DeepLinkHelper : NSObject

+ (void)handleGeoUrl:(NSURL *)url;
+ (void)handleFileUrl:(NSURL *)url;
+ (void)handleCommonUrl:(NSURL *)url;

@end

NS_ASSUME_NONNULL_END
