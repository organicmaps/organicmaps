#import "MWMAlert.h"

@interface MWMDefaultAlert : MWMAlert

+ (instancetype)routeNotFoundAlert;
+ (instancetype)routeNotFoundNoPublicTransportAlert;
+ (instancetype)routeNotFoundTooLongPedestrianAlert;
+ (instancetype)routeFileNotExistAlert;
+ (instancetype)endPointNotFoundAlert;
+ (instancetype)startPointNotFoundAlert;
+ (instancetype)intermediatePointNotFoundAlert;
+ (instancetype)internalRoutingErrorAlert;
+ (instancetype)incorrectFeaturePositionAlert;
+ (instancetype)internalErrorAlert;
+ (instancetype)notEnoughSpaceAlert;
+ (instancetype)invalidUserNameOrPasswordAlert;
+ (instancetype)noCurrentPositionAlert;
+ (instancetype)pointsInDifferentMWMAlert;
+ (instancetype)disabledLocationAlert;
+ (instancetype)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)noConnectionAlert;
+ (instancetype)migrationProhibitedAlert;
+ (instancetype)deleteMapProhibitedAlert;
+ (instancetype)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)locationServiceNotSupportedAlert;
+ (instancetype)point2PointAlertWithOkBlock:(MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (instancetype)disableAutoDownloadAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock
                                           cancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)downloaderNotEnoughSpaceAlert;
+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock
                                            cancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)downloaderNeedUpdateAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)routingMigrationAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)resetChangesAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)deleteFeatureAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block;
+ (instancetype)infoAlert:(NSString *)title text:(NSString *)text;
+ (instancetype)convertBookmarksWithCount:(NSUInteger)count okBlock:(MWMVoidBlock)okBlock;
+ (instancetype)bookmarkConversionErrorAlert;

+ (instancetype)restoreBookmarkAlertWithMessage:(NSString *)message
                              rightButtonAction:(MWMVoidBlock)rightButton
                               leftButtonAction:(MWMVoidBlock)leftButton;

+ (instancetype)tagsLoadingErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;

+ (instancetype)defaultAlertWithTitle:(NSString *)title
                              message:(NSString *)message
                     rightButtonTitle:(NSString *)rightButtonTitle
                      leftButtonTitle:(NSString *)leftButtonTitle
                    rightButtonAction:(MWMVoidBlock)action
                      statisticsEvent:(NSString *)statisticsEvent;

@end
