@interface MWMEditorHelper : NSObject

+ (BOOL)hasMapEditsOrNotesToUpload;
+ (BOOL)isOSMServerReachable;
+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler;

@end
