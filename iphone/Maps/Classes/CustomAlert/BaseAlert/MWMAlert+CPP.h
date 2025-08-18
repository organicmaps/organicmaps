#import "MWMAlert.h"

#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

using MWMDownloadBlock = void (^)(storage::CountriesVec const &, MWMVoidBlock);

@interface MWMAlert (CPP)

+ (MWMAlert *)alert:(routing::RouterResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::CountriesSet const &)countries
                                            code:(routing::RouterResultCode)code
                                     cancelBlock:(MWMVoidBlock)cancelBlock
                                   downloadBlock:(MWMDownloadBlock)downloadBlock
                           downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock;

@end
