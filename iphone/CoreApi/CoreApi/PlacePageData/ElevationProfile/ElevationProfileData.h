#import <Foundation/Foundation.h>
#import "ElevationHeightPoint.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, ElevationDifficulty) {
  ElevationDifficultyEasy,
  ElevationDifficultyModerate,
  ElevationDifficultyHard
};

@interface ElevationProfileData : NSObject

@property(nonatomic, readonly) uint64_t trackId;
@property(nonatomic, readonly) NSUInteger ascent;
@property(nonatomic, readonly) NSUInteger descent;
@property(nonatomic, readonly) NSUInteger maxAttitude;
@property(nonatomic, readonly) NSUInteger minAttitude;
@property(nonatomic, readonly) ElevationDifficulty difficulty;
@property(nonatomic, readonly) NSUInteger trackTime;
//@property(nonatomic, readonly, nullable) NSString *extendedDifficultyGrade;
//@property(nonatomic, readonly, nullable) NSString *extendedDifficultyDescription;
@property(nonatomic, readonly) NSArray<ElevationHeightPoint *> *points;
@property(nonatomic, readonly) double activePoint;
@property(nonatomic, readonly) double myPosition;

@end

NS_ASSUME_NONNULL_END
