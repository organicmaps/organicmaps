#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

@class MWMMapNodeAttributes;
@class MWMMapUpdateInfo;

NS_ASSUME_NONNULL_BEGIN

extern NSErrorDomain const kStorageErrorDomain;

extern NSInteger const kStorageNotEnoughSpace;
extern NSInteger const kStorageNoConnection;
extern NSInteger const kStorageCellularForbidden;
extern NSInteger const kStorageRoutingActive;
extern NSInteger const kStorageHaveUnsavedEdits;

NS_SWIFT_NAME(Storage)
@interface MWMStorage : NSObject

+ (BOOL)downloadNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error;
+ (void)retryDownloadNode:(NSString *)countryId;
+ (BOOL)updateNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error;
+ (BOOL)deleteNode:(NSString *)countryId
ignoreUnsavedEdits:(BOOL)force
             error:(NSError * __autoreleasing _Nullable *)error;
+ (void)cancelDownloadNode:(NSString *)countryId;
+ (void)showNode:(NSString *)countryId;
+ (BOOL)downloadNodes:(NSArray<NSString *> *)countryIds error:(NSError * __autoreleasing _Nullable *)error;

+ (BOOL)haveDownloadedCountries;
+ (BOOL)downloadInProgress;
+ (void)enableCellularDownload:(BOOL)enable;

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
