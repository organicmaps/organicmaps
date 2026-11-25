#import <Foundation/Foundation.h>

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

@interface RouteElevationPreviewData : NSObject

@property(nonatomic, readonly, nonnull) TrackInfo * trackInfo;
@property(nonatomic, readonly, nullable) ElevationProfileData * elevationProfileData;

/// Directed indexes into the full profile. A fractional entry interpolates adjacent samples.
@property(nonatomic, readonly) double entryPointIndex;
/// A negative finish denotes a profile without a followed section.
@property(nonatomic, readonly) NSInteger finishPointIndex;

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData;
- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData
                  entryPointIndex:(double)entryPointIndex
                 finishPointIndex:(NSInteger)finishPointIndex NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
