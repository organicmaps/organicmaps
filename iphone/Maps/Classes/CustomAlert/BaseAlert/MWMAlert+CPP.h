#import "MWMAlert.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

using MWMDownloadBlock = void (^)(storage::TCountriesVec const &, MWMVoidBlock);

@interface MWMAlert (CPP)

+ (MWMAlert *)alert:(routing::IRouter::ResultCode)type;
+ (MWMAlert *)downloaderAlertWithAbsentCountries:(storage::TCountriesVec const &)countries
                                            code:(routing::IRouter::ResultCode)code
                                     cancelBlock:(MWMVoidBlock)cancelBlock
                                   downloadBlock:(MWMDownloadBlock)downloadBlock
                           downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock;

@end
