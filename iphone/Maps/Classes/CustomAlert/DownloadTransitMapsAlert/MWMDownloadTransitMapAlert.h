#import "MWMAlert.h"

#include "storage/storage.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)downloaderAlertWithMaps:(storage::TCountriesVec const &)countries
                                   code:(routing::IRouter::ResultCode)code
                            cancelBlock:(TMWMVoidBlock)cancelBlock
                          downloadBlock:(TMWMDownloadBlock)downloadBlock
                  downloadCompleteBlock:(TMWMVoidBlock)downloadCompleteBlock;
- (void)showDownloadDetail:(UIButton *)sender;

@end
