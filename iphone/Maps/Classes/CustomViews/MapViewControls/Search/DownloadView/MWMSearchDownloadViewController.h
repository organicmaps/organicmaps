#import "ViewController.h"

@protocol MWMSearchDownloadProtocol <NSObject>

- (void)selectMapsAction;

@end

@interface MWMSearchDownloadViewController : ViewController

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull id<MWMSearchDownloadProtocol>)delegate;

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)setDownloadFailed;

@end
