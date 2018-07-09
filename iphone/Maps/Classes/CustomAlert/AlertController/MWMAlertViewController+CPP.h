#import "MWMAlert+CPP.h"
#import "MWMAlertViewController.h"

#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"
#include "storage/storage.hpp"

@interface MWMAlertViewController (CPP)

- (void)presentAlert:(routing::RouterResultCode)type;
- (void)presentDownloaderAlertWithCountries:(storage::TCountriesVec const &)countries
                                       code:(routing::RouterResultCode)code
                                cancelBlock:(nonnull MWMVoidBlock)cancelBlock
                              downloadBlock:(nonnull MWMDownloadBlock)downloadBlock
                      downloadCompleteBlock:(nonnull MWMVoidBlock)downloadCompleteBlock;

@end
