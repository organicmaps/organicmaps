#import "MWMDownloadBannerViewController.h"

@interface MWMDownloadBannerViewController ()

@property(copy, nonatomic) MWMVoidBlock tapHandler;

@end

@implementation MWMDownloadBannerViewController

- (instancetype)initWithTapHandler:(MWMVoidBlock)tapHandler {
  self = [super init];
  if (self) {
    _tapHandler = tapHandler;
  }
  return self;
}

- (IBAction)onButtonTap:(UIButton *)sender {
  if (self.tapHandler) {
    self.tapHandler();
  }
}

@end
