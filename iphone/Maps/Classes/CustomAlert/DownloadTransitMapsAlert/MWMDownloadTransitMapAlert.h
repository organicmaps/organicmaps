#import "MWMAlert.h"

#include "storage/storage.hpp"
#include "std/vector.hpp"

@interface MWMDownloadTransitMapAlert : MWMAlert

+ (instancetype)downloaderAlertWithMaps:(vector<storage::TIndex> const &)maps
                                 routes:(vector<storage::TIndex> const &)routes
                                   code:(routing::IRouter::ResultCode)code
                                  block:(TMWMVoidBlock)block;
- (void)showDownloadDetail:(UIButton *)sender;

@end
