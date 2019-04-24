#include "storage/storage_defines.hpp"

@interface MWMStorage : NSObject

+ (void)downloadNode:(storage::CountryId const &)countryId
           onSuccess:(MWMVoidBlock)onSuccess
            onCancel:(MWMVoidBlock)onCancel;
+ (void)retryDownloadNode:(storage::CountryId const &)countryId;
+ (void)updateNode:(storage::CountryId const &)countryId
          onCancel:(MWMVoidBlock)onCancel;
+ (void)deleteNode:(storage::CountryId const &)countryId;
+ (void)cancelDownloadNode:(storage::CountryId const &)countryId;
+ (void)showNode:(storage::CountryId const &)countryId;

+ (void)downloadNodes:(storage::CountriesVec const &)countryIds
            onSuccess:(MWMVoidBlock)onSuccess
             onCancel:(MWMVoidBlock)onCancel;

@end
