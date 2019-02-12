#include "storage/storage_defines.hpp"

@interface MWMStorage : NSObject

+ (void)downloadNode:(storage::CountryId const &)countryId onSuccess:(MWMVoidBlock)onSuccess;
+ (void)retryDownloadNode:(storage::CountryId const &)countryId;
+ (void)updateNode:(storage::CountryId const &)countryId;
+ (void)deleteNode:(storage::CountryId const &)countryId;
+ (void)cancelDownloadNode:(storage::CountryId const &)countryId;
+ (void)showNode:(storage::CountryId const &)countryId;

+ (void)downloadNodes:(storage::CountriesVec const &)countryIds onSuccess:(MWMVoidBlock)onSuccess;

@end
