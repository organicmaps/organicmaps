@interface MWMFrameworkHelper : NSObject

+ (void)processFirstLaunch;

+ (void)setVisibleViewport:(CGRect)rect;

+ (void)setTheme:(MWMTheme)theme;

+ (MWMDayTime)daytime;

@end
