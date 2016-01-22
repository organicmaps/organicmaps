#import "MWMAlert.h"
#import "ViewController.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController : ViewController

@property (weak, nonatomic, readonly) UIViewController * ownerViewController;

- (nonnull instancetype)initWithViewController:(nonnull UIViewController *)viewController;
- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentDownloaderAlertWithCountries:(vector<storage::TIndex> const &)countries
                                     routes:(vector<storage::TIndex> const &)routes
                                       code:(routing::IRouter::ResultCode)code;
- (void)presentRateAlert;
- (void)presentFacebookAlert;
- (void)presentPoint2PointAlertWithOkBlock:(nonnull TMWMVoidBlock)block needToRebuild:(BOOL)needToRebuild;
- (void)presentRoutingDisclaimerAlert;
- (void)presentDisabledLocationAlert;
- (void)presentLocationAlert;
- (void)presentLocationServiceNotSupportedAlert;
- (void)presentNoConnectionAlert;
- (void)presentnoWiFiAlertWithName:(nonnull NSString *)name downloadBlock:(nullable TMWMVoidBlock)block;
- (void)presentPedestrianToastAlert:(BOOL)isFirstLaunch;
- (void)closeAlertWithCompletion:(nullable TMWMVoidBlock)completion;

- (nonnull instancetype)init __attribute__((unavailable("call -initWithViewController: instead!")));
+ (nonnull instancetype)new __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithCoder:(nonnull NSCoder *)aDecoder __attribute__((unavailable("call -initWithViewController: instead!")));
- (nonnull instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil __attribute__((unavailable("call -initWithViewController: instead!")));
@end
