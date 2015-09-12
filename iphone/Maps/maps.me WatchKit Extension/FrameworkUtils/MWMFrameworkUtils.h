#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface MWMFrameworkUtils : NSObject

+ (void)prepareFramework;
+ (void)resetFramework;

+ (BOOL)hasMWM;
+ (NSString *)currentCountryName;

+ (void)initSoftwareRenderer;
+ (void)releaseSoftwareRenderer;
+ (UIImage *)getFrame:(CGSize)frameSize withZoomModifier:(int)zoomModifier;

+ (void)searchAroundCurrentLocation:(NSString *)query callback:(void(^)(NSMutableArray *result))reply;

@end