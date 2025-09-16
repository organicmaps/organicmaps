@interface MWMEditorHelper : NSObject

+ (BOOL)hasMapEditsOrNotesToUpload;
+ (void)uploadEditsWithTimeout:(NSTimeInterval)timeout completionHandler:(void (^)(UIBackgroundFetchResult))completionHandler;

@end
