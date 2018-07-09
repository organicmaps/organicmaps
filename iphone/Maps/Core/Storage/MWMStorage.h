#include "storage/index.hpp"

@interface MWMStorage : NSObject

+ (void)downloadNode:(storage::TCountryId const &)countryId onSuccess:(MWMVoidBlock)onSuccess;
+ (void)retryDownloadNode:(storage::TCountryId const &)countryId;
+ (void)updateNode:(storage::TCountryId const &)countryId;
+ (void)deleteNode:(storage::TCountryId const &)countryId;
+ (void)cancelDownloadNode:(storage::TCountryId const &)countryId;
+ (void)showNode:(storage::TCountryId const &)countryId;

+ (void)downloadNodes:(storage::TCountriesVec const &)countryIds onSuccess:(MWMVoidBlock)onSuccess;

@end
