@class MWMMapNodeAttributes;
@class MWMMapUpdateInfo;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(Storage)
@interface MWMStorage : NSObject

+ (void)downloadNode:(NSString *)countryId
           onSuccess:(nullable MWMVoidBlock)onSuccess
            onCancel:(nullable MWMVoidBlock)onCancel;
+ (void)retryDownloadNode:(NSString *)countryId;
+ (void)updateNode:(NSString *)countryId
          onCancel:(nullable MWMVoidBlock)onCancel;
+ (void)deleteNode:(NSString *)countryId;
+ (void)cancelDownloadNode:(NSString *)countryId;
+ (void)showNode:(NSString *)countryId;

+ (void)downloadNodes:(NSArray<NSString *> *)countryIds
            onSuccess:(nullable MWMVoidBlock)onSuccess
             onCancel:(nullable MWMVoidBlock)onCancel;

+ (BOOL)haveDownloadedCountries;
+ (BOOL)downloadInProgress;

#pragma mark - Attributes

+ (NSArray<NSString *> *)allCountries;
+ (NSArray<NSString *> *)allCountriesWithParent:(NSString *)countryId;
+ (NSArray<NSString *> *)availableCountriesWithParent:(NSString *)countryId;
+ (NSArray<NSString *> *)downloadedCountries;
+ (NSArray<NSString *> *)downloadedCountriesWithParent:(NSString *)countryId;
+ (MWMMapNodeAttributes *)attributesForCountry:(NSString *)countryId;
+ (MWMMapNodeAttributes *)attributesForRoot;
+ (NSString *)nameForCountry:(NSString *)countryId;
+ (nullable NSArray<NSString *> *)nearbyAvailableCountries:(CLLocationCoordinate2D)location;
+ (MWMMapUpdateInfo *)updateInfoWithParent:(nullable NSString *)countryId;

@end

NS_ASSUME_NONNULL_END
