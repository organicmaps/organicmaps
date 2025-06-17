#import <Foundation/Foundation.h>
#import "MWMTypes.h"

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackData : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) MWMMarkGroupID groupId;
@property(nonatomic, readwrite, nonnull) TrackInfo * trackInfo;
@property(nonatomic, readwrite, nullable) ElevationProfileData * elevationProfileData;
@property(nonatomic, readonly) double activePoint;
@property(nonatomic, readonly) double myPosition;
@property(nonatomic) MWMVoidBlock onActivePointChangedHandler;

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
             onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler;

- (void)updateActivePointDistance:(double)distance;

@end

NS_ASSUME_NONNULL_END
