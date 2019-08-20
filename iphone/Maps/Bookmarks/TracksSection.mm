#import "TracksSection.h"
#import "CircleView.h"
#include "Framework.h"

NS_ASSUME_NONNULL_BEGIN

namespace {
CGFloat const kPinDiameter = 22.0f;
}  // namespace

@interface TracksSection () {
  NSString *m_title;
  NSMutableArray<NSNumber *> *m_trackIds;
  BOOL m_isEditable;
}

@end

@implementation TracksSection

- (instancetype)initWithTitle:(nullable NSString *)title
                     trackIds:(MWMTrackIDCollection)trackIds
                   isEditable:(BOOL)isEditable {
  self = [super init];
  if (self) {
    m_title = title;
    m_trackIds = trackIds.mutableCopy;
    m_isEditable = isEditable;
  }
  return self;
}

- (kml::TrackId)getTrackIdForRow:(NSInteger)row {
  return static_cast<kml::TrackId>(m_trackIds[row].unsignedLongLongValue);
}

- (NSInteger)numberOfRows {
  return [m_trackIds count];
}

- (nullable NSString *)title {
  return m_title;
}

- (BOOL)canEdit {
  return m_isEditable;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
  if (!cell)
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
  CHECK(cell, ("Invalid track cell."));

  auto const &bm = GetFramework().GetBookmarkManager();

  auto const trackId = [self getTrackIdForRow:row];
  Track const *track = bm.GetTrack(trackId);
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

- (BOOL)didSelectRow:(NSInteger)row {
  auto const trackId = [self getTrackIdForRow:row];
  GetFramework().ShowTrack(trackId);
  return YES;
}

- (void)deleteRow:(NSInteger)row {
  auto const trackId = [self getTrackIdForRow:row];
  auto &bm = GetFramework().GetBookmarkManager();
  bm.GetEditSession().DeleteTrack(trackId);
  [m_trackIds removeObjectAtIndex:row];
}

@end

NS_ASSUME_NONNULL_END
