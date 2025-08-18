#import "MWMAlert.h"
#import "MWMMobileInternetAlert.h"
#import "MWMViewController.h"

@interface MWMAlertViewController : MWMViewController

+ (nonnull MWMAlertViewController *)activeAlertController;

@property(weak, nonatomic, readonly) UIViewController * _Null_unspecified ownerViewController;

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
- (void)presentRoutingDisclaimerAlertWithOkBlock:(nonnull nonnull MWMVoidBlock)block;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlertWithCancelBlock:(MWMVoidBlock _Nonnull)cancelBlock;
- (void)presentLocationServicesDisabledAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentNoConnectionAlert;
- (void)presentDeleteMapProhibitedAlert;
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentNoWiFiAlertWithOkBlock:(nullable MWMVoidBlock)okBlock andCancelBlock:(nullable MWMVoidBlock)cancelBlock;
- (void)presentIncorrectFeauturePositionAlert;
- (void)presentNotEnoughSpaceAlert;
- (void)presentInvalidUserNameOrPasswordAlert;
- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                          cancelBlock:(nonnull MWMVoidBlock)cancelBlock;
- (void)presentDownloaderNotEnoughSpaceAlert;
- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                           cancelBlock:(nonnull MWMVoidBlock)cancelBlock;
- (void)presentPlaceDoesntExistAlertWithBlock:(nonnull MWMStringBlock)block;
- (void)presentResetChangesAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentDeleteFeatureAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentEditorViralAlert;
- (void)presentOsmAuthAlert;
- (void)presentOsmReauthAlert;
- (void)presentPersonalInfoWarningAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentTrackWarningAlertWithCancelBlock:(nonnull MWMVoidBlock)block;
- (void)presentMobileInternetAlertWithBlock:(nonnull MWMMobileInternetAlertCompletionBlock)block;
- (void)presentInfoAlert:(nonnull NSString *)title text:(nonnull NSString *)text;
- (void)presentInfoAlert:(nonnull NSString *)title;
- (void)presentCreateBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                              minCharacterNum:(NSUInteger)min
                                                     callback:(nonnull MWMCheckStringBlock)callback;

- (void)presentBookmarkConversionErrorAlert;
- (void)presentBugReportAlertWithTitle:(nonnull NSString *)title;

- (void)presentDefaultAlertWithTitle:(nonnull NSString *)title
                             message:(nullable NSString *)message
                    rightButtonTitle:(nonnull NSString *)rightButtonTitle
                     leftButtonTitle:(nullable NSString *)leftButtonTitle
                   rightButtonAction:(nullable MWMVoidBlock)action;

- (void)closeAlert:(nullable MWMVoidBlock)completion;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype)new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder
    __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil
                                 bundle:(nullable NSBundle *)nibBundleOrNil
    __attribute__((unavailable("call -initWithViewController: instead!")));
@end
