#import <Foundation/Foundation.h>
#import <UIKit/UIColor.h>
#import "MWMTypes.h"

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

/// The user bookmark category a track belongs to. Absent (nil) for tracks that are not in any user
/// category, e.g. tracks rendered from OSM relations.
@interface PlacePageTrackCategory : NSObject

@property(nonatomic, readonly) MWMMarkGroupID groupId;
@property(nonatomic, readonly) NSString * name;

- (instancetype)initWithGroupId:(MWMMarkGroupID)groupId name:(NSString *)name NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

@interface PlacePageTrackData : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) BOOL isTempRelationTrack;
@property(nonatomic, readonly, nullable) PlacePageTrackCategory * category;
// TODO: The track description is not fully implemented in the core yet.
@property(nonatomic, readonly, nullable) NSString * trackDescription;

/// The color is nil for Track Recordings.
@property(nonatomic, readonly, nullable) UIColor * color;
@property(nonatomic, readwrite, nonnull) TrackInfo * trackInfo;
@property(nonatomic, readwrite, nullable) ElevationProfileData * elevationProfileData;
@property(nonatomic, readonly) double activePointDistance;
@property(nonatomic, readonly) double myPositionDistance;
@property(nonatomic) MWMVoidBlock onActivePointChangedHandler;

- (instancetype)initWithTrackInfo:(TrackInfo *)trackInfo
                    elevationInfo:(ElevationProfileData * _Nullable)elevationInfo
             onActivePointChanged:(MWMVoidBlock)onActivePointChangedHandler;

- (void)updateActivePointDistance:(double)distance;

@end

NS_ASSUME_NONNULL_END
