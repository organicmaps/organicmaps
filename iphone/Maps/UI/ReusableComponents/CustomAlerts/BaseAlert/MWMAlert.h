@class MWMAlertViewController;
@interface MWMAlert : UIView

@property(weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)locationAlertWithCancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)routingDisclaimerAlertWithOkBlock:(MWMVoidBlock)block;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)locationServicesDisabledAlert;
+ (MWMAlert *)noWiFiAlertWithOkBlock:(MWMVoidBlock)okBlock andCancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)deleteMapProhibitedAlert;
+ (MWMAlert *)unsavedEditsAlertWithOkBlock:(MWMVoidBlock)okBlock;
+ (MWMAlert *)locationServiceNotSupportedAlert;
+ (MWMAlert *)incorrectFeaturePositionAlert;
+ (MWMAlert *)notEnoughSpaceAlert;
+ (MWMAlert *)invalidUserNameOrPasswordAlert;
+ (MWMAlert *)point2PointAlertWithOkBlock:(MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)downloaderNotEnoughSpaceAlert;
+ (MWMAlert *)downloaderInternalErrorAlertWithOkBlock:(MWMVoidBlock)okBlock cancelBlock:(MWMVoidBlock)cancelBlock;
+ (MWMAlert *)placeDoesntExistAlertWithBlock:(MWMStringBlock)block;
+ (MWMAlert *)resetChangesAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)deleteFeatureAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)editorViralAlert;
+ (MWMAlert *)osmAuthAlert;
+ (MWMAlert *)osmReauthAlert;
+ (MWMAlert *)personalInfoWarningAlertWithBlock:(MWMVoidBlock)block;
+ (MWMAlert *)trackWarningAlertWithCancelBlock:(MWMVoidBlock)block;
+ (MWMAlert *)infoAlert:(NSString *)title text:(NSString *)text;
+ (MWMAlert *)createBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                             minCharacterNum:(NSUInteger)min
                                                    callback:(MWMCheckStringBlock)callback;
+ (MWMAlert *)spinnerAlertWithTitle:(NSString *)title cancel:(MWMVoidBlock)cancel;
+ (MWMAlert *)bookmarkConversionErrorAlert;
+ (MWMAlert *)tagsLoadingErrorAlertWithOkBlock:okBlock cancelBlock:cancelBlock;
+ (MWMAlert *)bugReportAlertWithTitle:(NSString *)title;

+ (MWMAlert *)defaultAlertWithTitle:(NSString *)title
                            message:(NSString *)message
                   rightButtonTitle:(NSString *)rightButtonTitle
                    leftButtonTitle:(NSString *)leftButtonTitle
                  rightButtonAction:(MWMVoidBlock)action;

- (void)close:(MWMVoidBlock)completion;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
