#import "ElevationProfileData.h"

@implementation ElevationProfileData

- (instancetype)init {
  self = [super init];
  if (self) {
    _serverId = @"0";
    _ascent = @"10000 m";
    _descent = @"20000 m";
    _maxAttitude = @"1213 m";
    _minAttitude = @"22134 m";
    _difficulty = ElevationDifficultyModerate;
    _trackTime = @"3h 12m";
    _extendedDifficultyGrade = @"S5";
    _extendedDifficultyDescription =
      @"On a S2 trail you will face bigger roots and stones (but never the Rolling Stones). Usually the ground isn’t "
      @"compact and you will find steps and stairs. Oftentimes there are narrow turns and the steepness in some spots "
      @"can be up to 70%.";

    NSMutableArray<ElevationHeightPoint *> *randPoints = [[NSMutableArray alloc] init];
    for (int i = 0; i < 100; i++) {
      [randPoints addObject:[[ElevationHeightPoint alloc] initWithDistance:arc4random() % 100
                                                                 andHeight:arc4random() % 200]];
    }
    _points = randPoints;
  }
  return self;
}

@end
