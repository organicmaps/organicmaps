#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, MWMMapNodeStatus) {
  MWMMapNodeStatusUndefined,
  MWMMapNodeStatusDownloading,
  MWMMapNodeStatusApplying,
  MWMMapNodeStatusInQueue,
  MWMMapNodeStatusError,
  MWMMapNodeStatusOnDiskOutOfDate,
  MWMMapNodeStatusOnDisk,
  MWMMapNodeStatusNotDownloaded,
  MWMMapNodeStatusPartly
} NS_SWIFT_NAME(MapNodeStatus);

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(CountryIdAndName)
@interface MWMCountryIdAndName : NSObject

@property(nonatomic, readonly) NSString * countryId;
@property(nonatomic, readonly) NSString * countryName;

- (instancetype)initWithCountryId:(NSString *)countryId name:(NSString *)countryName;

@end

// Mirrors storage::Storage::TerrainStatus.
typedef NS_ENUM(NSInteger, MWMTerrainStatus) {
  MWMTerrainStatusNotAvailable,
  MWMTerrainStatusNotDownloaded,
  MWMTerrainStatusDownloading,
  MWMTerrainStatusPartly,
  MWMTerrainStatusOnDisk,
  MWMTerrainStatusFailed,
} NS_SWIFT_NAME(TerrainStatus);

NS_SWIFT_NAME(MapNodeAttributes)
@interface MWMMapNodeAttributes : NSObject

@property(nonatomic, readonly) NSString * countryId;
@property(nonatomic, readonly) NSUInteger totalMwmCount;
@property(nonatomic, readonly) NSUInteger downloadedMwmCount;
@property(nonatomic, readonly) NSUInteger downloadingMwmCount;
@property(nonatomic, readonly) uint64_t totalSize;
@property(nonatomic, readonly) uint64_t downloadedSize;
@property(nonatomic, readonly) uint64_t downloadingSize;
@property(nonatomic, readonly) uint64_t totalUpdateSizeBytes;
@property(nonatomic, readonly) NSString * nodeName;
@property(nonatomic, readonly) NSString * nodeDescription;
@property(nonatomic, readonly) MWMMapNodeStatus nodeStatus;
@property(nonatomic, readonly) BOOL hasChildren;
@property(nonatomic, readonly) BOOL hasParent;
@property(nonatomic, readonly) NSArray<MWMCountryIdAndName *> * parentInfo;
@property(nonatomic, readonly, nullable) NSArray<MWMCountryIdAndName *> * topmostParentInfo;
@property(nonatomic, readonly) float downloadingProgress;

/// Terrain (.twm) coverage of the region, aggregated over the covering blocks.
@property(nonatomic, readonly) MWMTerrainStatus terrainStatus;
@property(nonatomic, readonly) uint64_t terrainTotalSize;
@property(nonatomic, readonly) uint64_t terrainDownloadedSize;

@end

NS_ASSUME_NONNULL_END
