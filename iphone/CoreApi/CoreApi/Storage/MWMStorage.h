#import <CoreLocation/CoreLocation.h>
#import <Foundation/Foundation.h>

@class MWMMapNodeAttributes;
@class MWMMapUpdateInfo;

NS_ASSUME_NONNULL_BEGIN

extern NSErrorDomain const kStorageErrorDomain;

extern NSInteger const kStorageNotEnoughSpace;
extern NSInteger const kStorageNoConnection;
extern NSInteger const kStorageCellularForbidden;
extern NSInteger const kStorageRoutingActive;
extern NSInteger const kStorageHaveUnsavedEdits;

NS_SWIFT_NAME(StorageObserver)
@protocol MWMStorageObserver <NSObject>

- (void)processCountryEvent:(NSString *)countryId;

@optional

- (void)processCountry:(NSString *)countryId downloadedBytes:(uint64_t)downloadedBytes totalBytes:(uint64_t)totalBytes;

@end

NS_SWIFT_NAME(Storage)
@interface MWMStorage : NSObject

+ (instancetype)sharedStorage;

- (BOOL)downloadNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error;
- (void)retryDownloadNode:(NSString *)countryId;
- (BOOL)updateNode:(NSString *)countryId error:(NSError * __autoreleasing _Nullable *)error;
- (BOOL)deleteNode:(NSString *)countryId
    ignoreUnsavedEdits:(BOOL)force
                 error:(NSError * __autoreleasing _Nullable *)error;
- (void)cancelDownloadNode:(NSString *)countryId;
- (void)showNode:(NSString *)countryId;
- (BOOL)downloadNodes:(NSArray<NSString *> *)countryIds error:(NSError * __autoreleasing _Nullable *)error;

- (BOOL)haveDownloadedCountries;
- (BOOL)downloadInProgress;
- (void)enableCellularDownload:(BOOL)enable;

- (void)addObserver:(id<MWMStorageObserver>)observer;
- (void)removeObserver:(id<MWMStorageObserver>)observer;

#pragma mark - Terrain

/// True when the bundle ships a terrain grid: the setting UI hides otherwise.
- (BOOL)isTerrainAvailable;
/// The "Download terrain with maps" setting (default ON): the terrain of a region
/// downloads, updates and deletes together with its map (see docs/TERRAIN.md).
- (BOOL)isTerrainWithMaps;
- (void)setTerrainWithMaps:(BOOL)enabled;
/// The bytes under the terrain directory, for the deletion confirmation.
- (uint64_t)terrainOnDiskSize;
/// Cancels every terrain download and removes all the terrain files.
- (void)deleteAllTerrain;
/// Downloads the terrain of the downloaded regions in the current viewport (the
/// "Enable" action of the terrain-disabled layer dialog).
/// NO on a connection error; NO without an error when the viewport holds no
/// downloaded region to fetch for.
- (BOOL)downloadTerrainForViewport:(NSError * _Nullable __autoreleasing * _Nullable)error;

#pragma mark - Attributes

- (NSArray<NSString *> *)allCountries;
- (NSArray<NSString *> *)allCountriesWithParent:(NSString *)countryId;
- (NSArray<NSString *> *)downloadedCountries;
- (NSArray<NSString *> *)downloadedCountriesWithParent:(NSString *)countryId;
- (MWMMapNodeAttributes *)attributesForCountry:(NSString *)countryId;
- (MWMMapNodeAttributes *)attributesForRoot;
- (NSString *)getRootId;
- (NSString *)nameForCountry:(NSString *)countryId;
- (nullable NSArray<NSString *> *)nearbyAvailableCountries:(CLLocationCoordinate2D)location;
- (nullable NSString *)countryForViewportCenter;
- (MWMMapUpdateInfo *)updateInfoWithParent:(nullable NSString *)countryId;

@end

NS_ASSUME_NONNULL_END
