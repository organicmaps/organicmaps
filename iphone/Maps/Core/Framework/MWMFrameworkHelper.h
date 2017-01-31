#import "MWMTypes.h"

@interface MWMFrameworkHelper : NSObject

+ (void)zoomToCurrentPosition;

+ (void)setVisibleViewport:(CGRect)rect;

+ (void)setTheme:(MWMTheme)theme;

+ (MWMDayTime)daytime;

@end
