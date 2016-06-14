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
                                cancelBlock:(nonnull TMWMVoidBlock)cancelBlock
                              downloadBlock:(nonnull TMWMDownloadBlock)downloadBlock
                      downloadCompleteBlock:(nonnull TMWMVoidBlock)downloadCompleteBlock;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
- (void)presentRoutingDisclaimerAlert;
- (void)presentBicycleRoutingDisclaimerAlert;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentLocationNotFoundAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentNoConnectionAlert;
- (void)presentMigrationProhibitedAlert;
- (void)presentDeleteMapProhibitedAlert;
- (void)presentUnsavedEditsAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentNoWiFiAlertWithOkBlock:(nullable TMWMVoidBlock)okBlock;
- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch;
- (void)presentIncorrectFeauturePositionAlert;
- (void)presentInternalErrorAlert;
- (void)presentNotEnoughSpaceAlert;
- (void)presentInvalidUserNameOrPasswordAlert;
- (void)presentDisableAutoDownloadAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentDownloaderNoConnectionAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock;
- (void)presentDownloaderNotEnoughSpaceAlert;
- (void)presentDownloaderInternalErrorAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock cancelBlock:(nonnull TMWMVoidBlock)cancelBlock;
- (void)presentDownloaderNeedUpdateAlertWithOkBlock:(nonnull TMWMVoidBlock)okBlock;
- (void)presentPlaceDoesntExistAlertWithBlock:(nonnull MWMStringBlock)block;
- (void)presentResetChangesAlertWithBlock:(nonnull TMWMVoidBlock)block;
- (void)presentDeleteFeatureAlertWithBlock:(nonnull TMWMVoidBlock)block;
- (void)presentEditorViralAlert;
- (void)presentOsmAuthAlert;
- (void)closeAlert;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype)new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithViewController: instead!")));
@end
