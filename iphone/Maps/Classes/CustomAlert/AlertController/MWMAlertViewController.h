#import "MWMAlert.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController : UIViewController

@property (weak, nonatomic, readonly) UIViewController * _Null_unspecified ownerViewController;

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController;
- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentRoutingMigrationAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentDownloaderAlertWithCountries:(storage::TCountriesVec const &)countries
                                       code:(routing::IRouter::ResultCode)code
                                    okBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
- (void)presentRoutingDisclaimerAlert;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentNoConnectionAlert;
- (void)presentMigrationProhibitedAlert;
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentNoWiFiAlertWithName:(nonnull NSString *)name okBlock:(nullable TMWMVoidBlock)okBlock;
- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch;
- (void)presentIncorrectFeauturePositionAlert;
- (void)presentInternalErrorAlert;
- (void)presentInvalidUserNameOrPasswordAlert;
- (void)presentDisableAutoDownloadAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock;
- (void)presentDownloaderNotEnoughSpaceAlert;
- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock;
- (void)presentDownloaderNeedUpdateAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentEditorViralAlert;
- (void)closeAlertWithCompletion:(nullable TMWMVoidBlock)completion;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype)new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithViewController: instead!")));
@end
