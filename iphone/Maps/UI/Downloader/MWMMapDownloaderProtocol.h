#include "storage/storage_defines.hpp"

@protocol MWMMapDownloaderProtocol <NSObject>

- (void)openNodeSubtree:(storage::CountryId const &)countryId;
- (void)downloadNode:(storage::CountryId const &)countryId;
- (void)retryDownloadNode:(storage::CountryId const &)countryId;
- (void)updateNode:(storage::CountryId const &)countryId;
- (void)deleteNode:(storage::CountryId const &)countryId;
- (void)cancelNode:(storage::CountryId const &)countryId;
- (void)showNode:(storage::CountryId const &)countryId;

@end
