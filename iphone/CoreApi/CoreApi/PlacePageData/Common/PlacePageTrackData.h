#import <Foundation/Foundation.h>
#import "MWMTypes.h"

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackData : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) MWMMarkGroupID groupId;
@property(nonatomic, readonly, nonnull) TrackInfo * trackInfo;
@property(nonatomic, readonly, nullable) ElevationProfileData * elevationProfileData;

- (instancetype)initWithTrackInfo:(TrackInfo * _Nonnull)trackInfo;

@end

NS_ASSUME_NONNULL_END
