@interface MWMEditorHelper : NSObject

+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler;

@end
