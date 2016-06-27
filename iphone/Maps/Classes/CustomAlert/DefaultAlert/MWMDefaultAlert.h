#import "MWMAlert.h"

@interface MWMDefaultAlert : MWMAlert

+ (instancetype)routeNotFoundAlert;
+ (instancetype)routeFileNotExistAlert;
+ (instancetype)endPointNotFoundAlert;
+ (instancetype)startPointNotFoundAlert;
+ (instancetype)internalRoutingErrorAlert;
+ (instancetype)incorrectFeauturePositionAlert;
+ (instancetype)internalErrorAlert;
+ (instancetype)notEnoughSpaceAlert;
+ (instancetype)invalidUserNameOrPasswordAlert;
+ (instancetype)noCurrentPositionAlert;
+ (instancetype)pointsInDifferentMWMAlert;
+ (instancetype)disabledLocationAlert;
+ (instancetype)noWiFiAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)noConnectionAlert;
+ (instancetype)migrationProhibitedAlert;
+ (instancetype)deleteMapProhibitedAlert;
+ (instancetype)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)locationServiceNotSupportedAlert;
+ (instancetype)locationNotFoundAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (instancetype)disableAutoDownloadAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock;
+ (instancetype)downloaderNotEnoughSpaceAlert;
+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock;
+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)routingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (instancetype)routingBicycleDisclaimerAlert;
+ (instancetype)resetChangesAlertWithBlock:(TMWMVoidBlock)block;
+ (instancetype)deleteFeatureAlertWithBlock:(TMWMVoidBlock)block;

@end
