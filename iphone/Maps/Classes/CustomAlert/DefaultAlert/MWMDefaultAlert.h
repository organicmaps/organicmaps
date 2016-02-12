#import "MWMAlert.h"

@interface MWMDefaultAlert : MWMAlert

+ (instancetype)routeNotFoundAlert;
+ (instancetype)routeFileNotExistAlert;
+ (instancetype)endPointNotFoundAlert;
+ (instancetype)startPointNotFoundAlert;
+ (instancetype)internalRoutingErrorAlert;
+ (instancetype)internalErrorAlert;
+ (instancetype)invalidUserNameOrPasswordAlert;
+ (instancetype)noCurrentPositionAlert;
+ (instancetype)pointsInDifferentMWMAlert;
+ (instancetype)disabledLocationAlert;
+ (instancetype)noWiFiAlertWithName:(NSString *)name okBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)noConnectionAlert;
+ (instancetype)locationServiceNotSupportedAlert;
+ (instancetype)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)downloaderNotEnoughSpaceAlert;
+ (instancetype)downloaderInternalErrorAlertForMap:(NSString *)name okBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock;

@end
