#import "MWMAlert+CPP.h"

#include "routing/routing_callbacks.hpp"

#include "storage/storage_defines.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)downloaderAlertWithMaps:(storage::CountriesSet const &)countries
                                   code:(routing::RouterResultCode)code
                            cancelBlock:(MWMVoidBlock)cancelBlock
                          downloadBlock:(MWMDownloadBlock)downloadBlock
                  downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock;
- (void)showDownloadDetail:(UIButton *)sender;

@end
