#include "routing/router.hpp"
#include "storage/storage.hpp"

@class MWMAlertViewController;
@interface MWMAlert : UIView

@property (weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(vector<storage::TIndex> const &)countries
                                          routes:(vector<storage::TIndex> const &)routes
                                            code:(routing::IRouter::ResultCode)code
                                           block:(TMWMVoidBlock)block;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlertWithInitialOrientation:(UIInterfaceOrientation)orientation;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name downloadBlock:(TMWMVoidBlock)block;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)locationServiceNotSupportedAlert;
+ (MWMAlert *)pedestrianToastShareAlert:(BOOL)isFirstLaunch;
+ (MWMAlert *)internalErrorAlert;
+ (MWMAlert *)invalidUserNameOrPasswordAlert;
+ (MWMAlert *)point2PointAlertWithOkBlock:(TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild;
+ (MWMAlert *)needMigrationAlertWithOkBlock:(TMWMVoidBlock)block;
- (void)close;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
