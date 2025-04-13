#import <UIKit/UIKit.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(Track)
@interface MWMTrack : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) NSString *trackName;
@property(nonatomic, readonly) NSInteger trackLengthMeters;
@property(nonatomic, readonly) UIColor *trackColor;

@end

NS_ASSUME_NONNULL_END
