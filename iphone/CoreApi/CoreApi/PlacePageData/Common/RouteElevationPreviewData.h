#import <Foundation/Foundation.h>

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

@interface RouteElevationPreviewData : NSObject

@property(nonatomic, readonly, nonnull) TrackInfo * trackInfo;
@property(nonatomic, readonly, nullable) ElevationProfileData * elevationProfileData;

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationProfileData NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
