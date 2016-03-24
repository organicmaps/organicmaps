#include "routing/router.hpp"
#include "storage/storage.hpp"

@class MWMAlertViewController;
@interface MWMAlert : UIView

@property (weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)routingMigrationAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::TCountriesVec const &)countries
                                            code:(routing::IRouter::ResultCode)code
                                         okBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlertWithInitialOrientation:(UIInterfaceOrientation)orientation;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name okBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)migrationProhibitedAlert;
+ (MWMAlert *)unsavedEditsAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)locationServiceNotSupportedAlert;
+ (MWMAlert *)pedestrianToastShareAlert:(BOOL)isFirstLaunch;
+ (MWMAlert *)incorrectFeauturePositionAlert;
+ (MWMAlert *)internalErrorAlert;
+ (MWMAlert *)invalidUserNameOrPasswordAlert;
+ (MWMAlert *)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (MWMAlert *)disableAutoDownloadAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock;
+ (MWMAlert *)downloaderNotEnoughSpaceAlert;
+ (MWMAlert *)downloaderInternalErrorAlertWithOkBlock:(TMWMVoidBlock)okBlock cancelBlock:(TMWMVoidBlock)cancelBlock;
+ (MWMAlert *)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)editorViralAlert;
- (void)close;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
