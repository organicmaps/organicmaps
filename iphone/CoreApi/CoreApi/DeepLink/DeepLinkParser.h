#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol IDeepLinkData;

@interface DeepLinkParser : NSObject

+ (id<IDeepLinkData>)parseAndSetApiURL:(NSURL*)url;
+ (bool)showMapForUrl:(NSURL*)url;
+ (void)addBookmarksFile:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END
