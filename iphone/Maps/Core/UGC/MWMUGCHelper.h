@interface MWMUGCHelper : NSObject

+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler;

@end
