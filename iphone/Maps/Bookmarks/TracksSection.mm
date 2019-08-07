#import "TracksSection.h"
#import "CircleView.h"
#include "Framework.h"

namespace
{
  CGFloat const kPinDiameter = 22.0f;
}  // namespace

@interface TracksSection()

@property (weak, nonatomic) id<TracksSectionDelegate> delegate;

@end

@implementation TracksSection

- (instancetype)initWithDelegate:(id<TracksSectionDelegate>)delegate
{
  return [self initWithBlockIndex:nil delegate:delegate];
}

- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate: (id<TracksSectionDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    _blockIndex = blockIndex;
    _delegate = delegate;
  }
  return self;
}

- (NSInteger)numberOfRows
{
  return [self.delegate numberOfTracksInSection:self];
}

- (NSString *)title
{
  return [self.delegate titleOfTracksSection:self];
}

- (BOOL)canEdit
{
  return [self.delegate canEditTracksSection:self];
}

- (UITableViewCell *)tableView: (UITableView *)tableView cellForRow: (NSInteger)row
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
  if (!cell)
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
  CHECK(cell, ("Invalid track cell."));
  
  auto const & bm = GetFramework().GetBookmarkManager();
  
  kml::TrackId const trackId = [self.delegate tracksSection:self getTrackIdByRow:row];
  Track const * track = bm.GetTrack(trackId);
  cell.textLabel.text = @(track->GetName().c_str());
  string dist;
  if (measurement_utils::FormatDistance(track->GetLengthMeters(), dist))
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"length"), @(dist.c_str())];
  else
    cell.detailTextLabel.text = nil;
  dp::Color const c = track->GetColor(0);
  cell.imageView.image = [CircleView createCircleImageWith:kPinDiameter
                                                  andColor:[UIColor colorWithRed:c.GetRedF()
                                                                           green:c.GetGreenF()
                                                                            blue:c.GetBlueF()
                                                                           alpha:1.f]];
  return cell;
}

- (BOOL)didSelectRow: (NSInteger)row
{
  kml::TrackId const trackId = [self.delegate tracksSection:self getTrackIdByRow:row];
  GetFramework().ShowTrack(trackId);
  return YES;
}

- (BOOL)deleteRow: (NSInteger)row
{
  kml::TrackId const trackId = [self.delegate tracksSection:self getTrackIdByRow:row];
  auto & bm = GetFramework().GetBookmarkManager();
  bm.GetEditSession().DeleteTrack(trackId);
  return [self.delegate tracksSection:self onDeleteTrackInRow:row];
}

@end
