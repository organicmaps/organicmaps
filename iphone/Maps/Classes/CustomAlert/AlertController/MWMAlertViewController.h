#import "MWMAlert.h"
#import "MWMViewController.h"

@interface MWMAlertViewController : MWMViewController

+ (nonnull MWMAlertViewController *)activeAlertController;

@property(weak, nonatomic, readonly) UIViewController * _Null_unspecified ownerViewController;

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController;
- (void)presentRoutingMigrationAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                             needToRebuild:(BOOL)needToRebuild;
- (void)presentRoutingDisclaimerAlertWithOkBlock:(nonnull nonnull MWMVoidBlock)block;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentLocationNotFoundAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentNoConnectionAlert;
- (void)presentMigrationProhibitedAlert;
- (void)presentDeleteMapProhibitedAlert;
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentNoWiFiAlertWithOkBlock:(nullable MWMVoidBlock)okBlock;
- (void)presentIncorrectFeauturePositionAlert;
- (void)presentInternalErrorAlert;
- (void)presentNotEnoughSpaceAlert;
- (void)presentInvalidUserNameOrPasswordAlert;
- (void)presentDisableAutoDownloadAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                          cancelBlock:(nonnull MWMVoidBlock)cancelBlock;
- (void)presentDownloaderNotEnoughSpaceAlert;
- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                           cancelBlock:(nonnull MWMVoidBlock)cancelBlock;
- (void)presentDownloaderNeedUpdateAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock;
- (void)presentPlaceDoesntExistAlertWithBlock:(nonnull MWMStringBlock)block;
- (void)presentResetChangesAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentDeleteFeatureAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentEditorViralAlert;
- (void)presentOsmAuthAlert;
- (void)presentPersonalInfoWarningAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentTrackWarningAlertWithCancelBlock:(nonnull MWMVoidBlock)block;
- (void)presentSearchNoResultsAlert;
- (void)presentMobileInternetAlertWithBlock:(nonnull MWMVoidBlock)block;
- (void)presentInfoAlert:(nonnull NSString *)title text:(nonnull NSString *)text;
- (void)presentCreateBookmarkCategoryAlertWithMaxCharacterNum:(NSUInteger)max
                                              minCharacterNum:(NSUInteger)min
                                                     callback:(nonnull MWMCheckStringBlock)callback;

- (void)presentConvertBookmarksAlertWithCount:(NSUInteger)count block:(nonnull MWMVoidBlock)block;

- (void)presentSpinnerAlertWithTitle:(nonnull NSString *)title cancel:(nullable MWMVoidBlock)cancel;
- (void)presentBookmarkConversionErrorAlert;

- (void)presentRestoreBookmarkAlertWithMessage:(nonnull NSString *)message
                             rightButtonAction:(nonnull MWMVoidBlock)rightButton
                              leftButtonAction:(nonnull MWMVoidBlock)leftButton;

- (void)presentTagsLoadingErrorAlertWithOkBlock:(nonnull MWMVoidBlock)okBlock
                                    cancelBlock:(nonnull MWMVoidBlock)cancelBlock;

- (void)presentDefaultAlertWithTitle:(nonnull NSString *)title
                             message:(nullable NSString *)message
                    rightButtonTitle:(nonnull NSString *)rightButtonTitle
                     leftButtonTitle:(nullable NSString *)leftButtonTitle
                   rightButtonAction:(nullable MWMVoidBlock)action;


- (void)closeAlert:(nullable MWMVoidBlock)completion;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype) new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder
    __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil
                                 bundle:(nullable NSBundle *)nibBundleOrNil
    __attribute__((unavailable("call -initWithViewController: instead!")));
@end
