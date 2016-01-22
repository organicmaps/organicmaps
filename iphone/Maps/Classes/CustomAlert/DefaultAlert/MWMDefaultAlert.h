#import "MWMAlert.h"

@interface MWMDefaultAlert : MWMAlert

+ (instancetype)routeNotFoundAlert;
+ (instancetype)routeFileNotExistAlert;
+ (instancetype)endPointNotFoundAlert;
+ (instancetype)startPointNotFoundAlert;
+ (instancetype)internalErrorAlert;
+ (instancetype)noCurrentPositionAlert;
+ (instancetype)pointsInDifferentMWMAlert;
+ (instancetype)disabledLocationAlert;
+ (instancetype)noWiFiAlertWithName:(NSString *)name downloadBlock:(TMWMVoidBlock)block;
+ (instancetype)noConnectionAlert;
+ (instancetype)locationServiceNotSupportedAlert;
+ (instancetype)point2PointAlertWithOkBlock:(TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild;

@end
