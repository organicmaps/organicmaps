@interface MWMEditorHelper : NSObject

+ (BOOL)hasMapEditsOrNotesToUpload;
+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler;

@end
