#import "MWMAlert.h"
#import "MWMViewController.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController : MWMViewController

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController;
- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentDownloaderAlertWithCountries:(storage::TCountriesVec const &)countries
                                     routes:(storage::TCountriesVec const &)routes
                                       code:(routing::IRouter::ResultCode)code
                                      block:(nonnull TMWMVoidBlock)block;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild;
- (void)presentUpdateMapsAlertWithOkBlock:(nonnull TMWMVoidBlock)block;
- (void)presentRoutingDisclaimerAlert;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentNoConnectionAlert;
- (void)presentNoWiFiAlertWithName:(nonnull NSString *)name downloadBlock:(nullable TMWMVoidBlock)block;
- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch;
- (void)presentInternalErrorAlert;
- (void)presentInvalidUserNameOrPasswordAlert;
- (void)closeAlertWithCompletion:(nullable TMWMVoidBlock)completion;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype)new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithViewController: instead!")));
@end
