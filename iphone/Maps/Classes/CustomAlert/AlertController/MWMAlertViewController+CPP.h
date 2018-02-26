#import "MWMAlert+CPP.h"
#import "MWMAlertViewController.h"

#include "routing/router.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController (CPP)

- (void)presentAlert:(routing::IRouter::ResultCode)type;
- (void)presentDownloaderAlertWithCountries:(storage::TCountriesVec const &)countries
                                       code:(routing::IRouter::ResultCode)code
                                cancelBlock:(nonnull MWMVoidBlock)cancelBlock
                              downloadBlock:(nonnull MWMDownloadBlock)downloadBlock
                      downloadCompleteBlock:(nonnull MWMVoidBlock)downloadCompleteBlock;

@end
