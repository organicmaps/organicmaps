#import <Foundation/Foundation.h>
#import <UIKit/UIColor.h>
#import "MWMTypes.h"

@class TrackInfo;
@class ElevationProfileData;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackData : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) MWMMarkGroupID groupId;
@property(nonatomic, readonly, nullable) NSString * trackCategory;
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
