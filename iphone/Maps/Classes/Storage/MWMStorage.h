#import "MWMAlertViewController.h"

#include "storage/index.hpp"

@interface MWMStorage : NSObject

+ (void)downloadNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess;
+ (void)retryDownloadNode:(storage::TCountryId const &)countryId;
+ (void)updateNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController;
+ (void)deleteNode:(storage::TCountryId const &)countryId;
+ (void)cancelDownloadNode:(storage::TCountryId const &)countryId;

@end
