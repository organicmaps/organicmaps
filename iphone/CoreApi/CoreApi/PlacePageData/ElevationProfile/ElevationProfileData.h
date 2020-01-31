#import <Foundation/Foundation.h>
#import "ElevationHeightPoint.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, ElevationDifficulty) {
  ElevationDifficultyEasy,
  ElevationDifficultyModerate,
  ElevationDifficultyHard
};

@interface ElevationProfileData : NSObject

@property(nonatomic, readonly) NSString* serverId;
@property(nonatomic, readonly) NSString* ascent;
@property(nonatomic, readonly) NSString* descent;
@property(nonatomic, readonly) NSString* maxAttitude;
@property(nonatomic, readonly) NSString* minAttitude;
@property(nonatomic, readonly) ElevationDifficulty difficulty;
@property(nonatomic, readonly) NSString* trackTime;
@property(nonatomic, readonly, nullable) NSString* extendedDifficultyGrade;
@property(nonatomic, readonly, nullable) NSString* extendedDifficultyDescription;
@property(nonatomic, readonly) NSArray<ElevationHeightPoint*>* points;

@end

NS_ASSUME_NONNULL_END
