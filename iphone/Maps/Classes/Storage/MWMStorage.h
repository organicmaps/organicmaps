#import "MWMAlertViewController.h"

#include "storage/index.hpp"

@interface MWMStorage : NSObject

+ (void)startSession;
+ (void)downloadNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess;
+ (void)retryDownloadNode:(storage::TCountryId const &)countryId;
+ (void)updateNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController;
+ (void)deleteNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController;
+ (void)cancelDownloadNode:(storage::TCountryId const &)countryId;
+ (void)showNode:(storage::TCountryId const &)countryId;

+ (void)downloadNodes:(storage::TCountriesVec const &)countryIds alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess;

@end
