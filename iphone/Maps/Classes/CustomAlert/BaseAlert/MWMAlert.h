#include "routing/router.hpp"
#include "storage/storage.hpp"

@class MWMAlertViewController;
@interface MWMAlert : UIView

@property (weak, nonatomic) MWMAlertViewController * alertController;

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::TCountriesVec const &)countries
                                          routes:(storage::TCountriesVec const &)routes
                                            code:(routing::IRouter::ResultCode)code
                                           block:(TMWMVoidBlock)block;
+ (MWMAlert *)rateAlert;
+ (MWMAlert *)facebookAlert;
+ (MWMAlert *)locationAlert;
+ (MWMAlert *)routingDisclaimerAlertWithInitialOrientation:(UIInterfaceOrientation)orientation;
+ (MWMAlert *)disabledLocationAlert;
+ (MWMAlert *)noWiFiAlertWithName:(NSString *)name okBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)noConnectionAlert;
+ (MWMAlert *)locationServiceNotSupportedAlert;
+ (MWMAlert *)pedestrianToastShareAlert:(BOOL)isFirstLaunch;
+ (MWMAlert *)internalErrorAlert;
+ (MWMAlert *)invalidUserNameOrPasswordAlert;
+ (MWMAlert *)point2PointAlertWithOkBlock:(TMWMVoidBlock)okBlock needToRebuild:(BOOL)needToRebuild;
+ (MWMAlert *)downloaderNoConnectionAlertWithOkBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)downloaderNotEnoughSpaceAlert;
+ (MWMAlert *)downloaderInternalErrorAlertForMap:(NSString *)name okBlock:(TMWMVoidBlock)okBlock;
+ (MWMAlert *)downloaderNeedUpdateAlertWithOkBlock:(TMWMVoidBlock)okBlock;
- (void)close;

- (void)setNeedsCloseAlertAfterEnterBackground;
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation;

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;

- (void)addControllerViewToWindow;

@end
