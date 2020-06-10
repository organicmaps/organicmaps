#import "TracksSection.h"
#import <CoreApi/MWMBookmarksManager.h>
#import "legacy_bookmark_colors.h"
#import "Statistics.h"

#include <CoreApi/Framework.h>

namespace {
CGFloat const kPinDiameter = 22.0f;
}  // namespace

@interface TracksSection ()

@property(copy, nonatomic, nullable) NSString *sectionTitle;
@property(strong, nonatomic) NSMutableArray<NSNumber *> *trackIds;
@property(nonatomic) BOOL isEditable;

@end

@implementation TracksSection

- (instancetype)initWithTitle:(nullable NSString *)title
                     trackIds:(MWMTrackIDCollection)trackIds
                   isEditable:(BOOL)isEditable {
  self = [super init];
  if (self) {
    _sectionTitle = [title copy];
    _trackIds = [trackIds mutableCopy];
    _isEditable = isEditable;
  }
  return self;
}

- (kml::TrackId)trackIdForRow:(NSInteger)row {
  return static_cast<kml::TrackId>(self.trackIds[row].unsignedLongLongValue);
}

- (NSInteger)numberOfRows {
  return [self.trackIds count];
}

- (nullable NSString *)title {
  return self.sectionTitle;
}

- (BOOL)canEdit {
  return self.isEditable;
}

- (BOOL)canSelect {
  return YES;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
  if (!cell)
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
  CHECK(cell, ("Invalid track cell."));

  auto const &bm = GetFramework().GetBookmarkManager();

  auto const trackId = [self trackIdForRow:row];
  Track const *track = bm.GetTrack(trackId);
  cell.textLabel.text = @(track->GetName().c_str());
  std::string dist;
  if (measurement_utils::FormatDistance(track->GetLengthMeters(), dist))
    cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"length"), @(dist.c_str())];
  else
    cell.detailTextLabel.text = nil;
  dp::Color const c = track->GetColor(0);
  cell.imageView.image = ios_bookmark_ui_helper::ImageForTrack(c.GetRedF(), c.GetGreenF(), c.GetBlueF());
  return cell;
}

- (void)didSelectRow:(NSInteger)row {
  auto const trackId = [self trackIdForRow:row];
  
  auto const track = GetFramework().GetBookmarkManager().GetTrack(trackId);
  if (track != nullptr && [[MWMBookmarksManager sharedManager] isGuide:track->GetGroupId()]) {
    [Statistics logEvent:kStatGuidesTrackSelect
          withParameters:@{kStatServerId: [[MWMBookmarksManager sharedManager] getServerId:track->GetGroupId()]}
             withChannel:StatisticsChannelRealtime];
  }
  
  GetFramework().ShowTrack(trackId);
}

- (void)deleteRow:(NSInteger)row {
  auto const trackId = [self trackIdForRow:row];
  auto &bm = GetFramework().GetBookmarkManager();
  bm.GetEditSession().DeleteTrack(trackId);
  [self.trackIds removeObjectAtIndex:row];
}

@end
