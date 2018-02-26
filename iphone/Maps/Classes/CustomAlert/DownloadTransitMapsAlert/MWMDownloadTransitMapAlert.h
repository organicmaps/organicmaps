#import "MWMAlert+CPP.h"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)downloaderAlertWithMaps:(storage::TCountriesVec const &)countries
                                   code:(routing::IRouter::ResultCode)code
                            cancelBlock:(MWMVoidBlock)cancelBlock
                          downloadBlock:(MWMDownloadBlock)downloadBlock
                  downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock;
- (void)showDownloadDetail:(UIButton *)sender;

@end
