@class MWMAlertViewController;
@interface MWMAlert : UIView

@property(weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)routingMigrationAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)migrationProhibitedAlert;
+ (MWMAlert *)deleteMapProhibitedAlert;
+ (MWMAlert *)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)locationServiceNotSupportedAlert;
+ (MWMAlert *)incorrectFeaturePositionAlert;
+ (MWMAlert *)internalErrorAlert;
+ (MWMAlert *)notEnoughSpaceAlert;
+ (MWMAlert *)invalidUserNameOrPasswordAlert;
+ (MWMAlert *)point2PointAlertWithOkBlock:(MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (MWMAlert *)disableAutoDownloadAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock
                                         cancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)downloaderNotEnoughSpaceAlert;
+ (MWMAlert *)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock
                                          cancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)downloaderNeedUpdateAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block;
+ (MWMAlert *)resetChangesAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)deleteFeatureAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)editorViralAlert;
+ (MWMAlert *)osmAuthAlert;
+ (MWMAlert *)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block;
+ (MWMAlert *)infoAlert:(NSString *)title text:(NSString *)text;
- (void)close:(MWMVoidBlock)completion;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
