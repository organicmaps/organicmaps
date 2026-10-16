#import "ElevationProfileData+Core.h"

#include "map/track_elevation.hpp"

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
{
  uint64_t m_recordingEpoch;
  size_t m_recordingRevision;
}

@end

@implementation ElevationProfileData (Core)

- (instancetype)initWithTrackId:(MWMTrackID)trackId elevationInfo:(ElevationInfo const &)elevationInfo
{
  return [self initWithTrackId:trackId elevationInfo:elevationInfo isCancelled:^BOOL { return NO; }];
}

- (nullable instancetype)initWithTrackId:(MWMTrackID)trackId
                           elevationInfo:(ElevationInfo const &)elevationInfo
                             isCancelled:(ElevationProfileCancellation)isCancelled
{
  self = [super init];
  if (self)
  {
    _trackId = trackId;
    _difficulty = convertDifficulty(elevationInfo.GetDifficulty());
    if (![ElevationProfileData fillPoints:&_points
                         segmentDistances:&_segmentDistances
                        fromElevationInfo:elevationInfo
                              isCancelled:isCancelled])
      return nil;
    _isTrackRecording = false;
  }
  return self;
}

- (nullable instancetype)initWithRecordingSnapshot:(GpsTrack::ElevationSnapshot const &)snapshot
                                       isCancelled:(ElevationProfileCancellation)isCancelled
{
  self = [self initWithTrackId:0 elevationInfo:snapshot.m_elevation isCancelled:isCancelled];
  if (self)
  {
    _isTrackRecording = true;
    m_recordingEpoch = snapshot.m_epoch;
    m_recordingRevision = snapshot.m_revision;
  }
  return self;
}

- (BOOL)matchesRecordingSnapshot:(GpsTrack::ElevationSnapshot const &)snapshot
{
  return _isTrackRecording && m_recordingEpoch == snapshot.m_epoch && m_recordingRevision == snapshot.m_revision;
}

+ (BOOL)fillPoints:(NSArray<ElevationHeightPoint *> * __strong *)outPoints
     segmentDistances:(NSArray<NSNumber *> * __strong *)outSegmentDistances
    fromElevationInfo:(ElevationInfo const &)elevationInfo
          isCancelled:(ElevationProfileCancellation)isCancelled
{
  NSMutableArray * pointsArray = [[NSMutableArray alloc] initWithCapacity:elevationInfo.GetSize()];
  NSMutableArray<NSNumber *> * distancesArray = [[NSMutableArray alloc] init];
  double cumulativeOffset = 0;
  size_t count = 0;
  auto const & lines = elevationInfo.GetLines();
  for (size_t i = 0; i < lines.size(); ++i)
  {
    if (i > 0)
      [distancesArray addObject:@(cumulativeOffset)];
    for (auto const & point : lines[i])
    {
      if (count++ % TrackElevation::kWindowIntervals == 0 && isCancelled())
        return NO;
      [pointsArray addObject:[[ElevationHeightPoint alloc] initWithDistance:(cumulativeOffset + point.m_distance)
                                                                   altitude:point.m_altitude]];
    }
    if (!lines[i].empty())
      cumulativeOffset += lines[i].back().m_distance;
  }
  *outPoints = pointsArray;
  *outSegmentDistances = distancesArray;
  return YES;
}

@end
