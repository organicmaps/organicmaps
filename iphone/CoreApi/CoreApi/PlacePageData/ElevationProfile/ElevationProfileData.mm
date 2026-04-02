#import "ElevationProfileData+Core.h"

static ElevationDifficulty convertDifficulty(uint8_t difficulty)
{
  switch (difficulty)
  {
  case ElevationInfo::Difficulty::Easy: return ElevationDifficulty::ElevationDifficultyEasy;
  case ElevationInfo::Difficulty::Medium: return ElevationDifficulty::ElevationDifficultyMedium;
  case ElevationInfo::Difficulty::Hard: return ElevationDifficulty::ElevationDifficultyHard;
  case ElevationInfo::Difficulty::Unknown: return ElevationDifficulty::ElevationDifficultyDisabled;
  }
  return ElevationDifficulty::ElevationDifficultyDisabled;
}

@implementation ElevationProfileData

@end

@implementation ElevationProfileData (Core)

- (instancetype)initWithTrackId:(MWMTrackID)trackId elevationInfo:(ElevationInfo const &)elevationInfo
{
  self = [super init];
  if (self)
  {
    _trackId = trackId;
    _difficulty = convertDifficulty(elevationInfo.GetDifficulty());
    [ElevationProfileData fillPoints:&_points segmentDistances:&_segmentDistances fromElevationInfo:elevationInfo];
    _isTrackRecording = false;
  }
  return self;
}

- (instancetype)initWithElevationInfo:(ElevationInfo const &)elevationInfo
{
  self = [super init];
  if (self)
  {
    _difficulty = convertDifficulty(elevationInfo.GetDifficulty());
    [ElevationProfileData fillPoints:&_points segmentDistances:&_segmentDistances fromElevationInfo:elevationInfo];
    _isTrackRecording = true;
  }
  return self;
}

+ (void)fillPoints:(NSArray<ElevationHeightPoint *> * __strong *)outPoints
     segmentDistances:(NSArray<NSNumber *> * __strong *)outSegmentDistances
    fromElevationInfo:(ElevationInfo const &)elevationInfo
{
  NSMutableArray * pointsArray = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> * distancesArray = [[NSMutableArray alloc] init];

  double cumulativeOffset = 0;
  auto const & lines = elevationInfo.GetLines();
  for (size_t i = 0; i < lines.size(); ++i)
  {
    if (i > 0)
      [distancesArray addObject:@(cumulativeOffset)];

    for (auto const & point : lines[i])
    {
      ElevationHeightPoint * elevationPoint =
          [[ElevationHeightPoint alloc] initWithDistance:(cumulativeOffset + point.m_distance)
                                                altitude:point.m_altitude];
      [pointsArray addObject:elevationPoint];
    }

    if (!lines[i].empty())
      cumulativeOffset += lines[i].back().m_distance;
  }

  *outPoints = pointsArray;
  *outSegmentDistances = distancesArray;
}

@end
