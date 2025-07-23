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
+ (instancetype)notEnoughSpaceAlert;
+ (instancetype)invalidUserNameOrPasswordAlert;
+ (instancetype)noCurrentPositionAlert;
+ (instancetype)pointsInDifferentMWMAlert;
+ (instancetype)disabledLocationAlert;
+ (instancetype)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock andCancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)noConnectionAlert;
+ (instancetype)deleteMapProhibitedAlert;
+ (instancetype)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (instancetype)locationServiceNotSupportedAlert;
+ (instancetype)point2PointAlertWithOkBlock:(MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (instancetype)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)downloaderNotEnoughSpaceAlert;
+ (instancetype)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)resetChangesAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)deleteFeatureAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block;
+ (instancetype)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block;
+ (instancetype)infoAlert:(NSString *)title text:(NSString *)text;
+ (instancetype)convertBookmarksWithCount:(NSUInteger)count okBlock:(MWMVoidBlock)okBlock;
+ (instancetype)bookmarkConversionErrorAlert;
+ (instancetype)tagsLoadingErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;
+ (instancetype)bugReportAlertWithTitle:(NSString *)title;

+ (instancetype)defaultAlertWithTitle:(NSString *)title
                              message:(NSString *)message
                     rightButtonTitle:(NSString *)rightButtonTitle
                      leftButtonTitle:(NSString *)leftButtonTitle
                    rightButtonAction:(MWMVoidBlock)action
                                  log:(NSString *)log;

@end
