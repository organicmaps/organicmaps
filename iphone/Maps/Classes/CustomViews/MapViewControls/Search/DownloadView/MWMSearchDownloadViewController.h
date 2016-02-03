#import "MWMViewController.h"

@protocol MWMSearchDownloadProtocol <NSObject>

@property (nonnull, nonatomic, readonly) MWMAlertViewController * alertController;

- (void)selectMapsAction;

@end

@interface MWMSearchDownloadViewController : MWMViewController

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull id<MWMSearchDownloadProtocol>)delegate;

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)setDownloadFailed;

@end
