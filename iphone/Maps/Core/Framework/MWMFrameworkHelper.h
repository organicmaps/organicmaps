typedef NS_ENUM(NSUInteger, MWMZoomMode) {
  MWMZoomModeIn = 0,
  MWMZoomModeOut
};

NS_SWIFT_NAME(FrameworkHelper)
@interface MWMFrameworkHelper : NSObject

+ (void)processFirstLaunch;

+ (void)setVisibleViewport:(CGRect)rect;

+ (void)setTheme:(MWMTheme)theme;

+ (MWMDayTime)daytime;

+ (void)checkConnectionAndPerformAction:(MWMVoidBlock)action cancelAction:(MWMVoidBlock)cancel;

+ (void)createFramework;

+ (BOOL)canUseNetwork;

+ (BOOL)isNetworkConnected;

+ (BOOL)isWiFiConnected;

+ (MWMMarkGroupID)invalidCategoryId;

+ (void)zoomMap:(MWMZoomMode)mode;
+ (void)moveMap:(UIOffset)offset;
+ (void)deactivateMapSelection:(BOOL)notifyUI NS_SWIFT_NAME(deactivateMapSelection(notifyUI:));
+ (void)switchMyPositionMode;
+ (void)stopLocationFollow;
+ (NSArray<NSString *> *)obtainLastSearchQueries;
+ (BOOL)needUpdateMaps;

@end
