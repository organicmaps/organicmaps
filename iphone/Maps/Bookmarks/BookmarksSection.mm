#import "BookmarksSection.h"
#import "legacy_bookmark_colors.h"
#import "MWMLocationHelpers.h"
#import "MWMSearchManager.h"

#import <CoreApi/CoreApi.h>

#include "geometry/distance_on_sphere.hpp"

@interface BookmarksSection ()

@property(copy, nonatomic, nullable) NSString *sectionTitle;
@property(strong, nonatomic) NSMutableArray<NSNumber *> *markIds;
@property(nonatomic) BOOL isEditable;

@end

@implementation BookmarksSection

- (instancetype)initWithTitle:(nullable NSString *)title
                      markIds:(MWMMarkIDCollection)markIds
                   isEditable:(BOOL)isEditable {
  self = [super init];
  if (self) {
    _sectionTitle = [title copy];
    _markIds = [markIds mutableCopy];
    _isEditable = isEditable;
  }
  return self;
}

- (kml::MarkId)markIdForRow:(NSInteger)row {
  return static_cast<kml::MarkId>(self.markIds[row].unsignedLongLongValue);
}

- (NSInteger)numberOfRows {
  return [self.markIds count];
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

- (void)fillCell:(UITableViewCell *)cell
  withBookmarkDetails:(Bookmark const *)bookmark
          andLocation:(CLLocation *)location {
  std::vector<std::string> details;

  if (location) {
    m2::PointD const pos = bookmark->GetPivot();
    double const meters = ms::DistanceOnEarth(location.coordinate.latitude, location.coordinate.longitude,
                                              mercator::YToLat(pos.y), mercator::XToLon(pos.x));
    details.push_back(location_helpers::formattedDistance(meters).UTF8String);
  }

  auto const &types = bookmark->GetData().m_featureTypes;
  if (!types.empty())
    details.push_back(kml::GetLocalizedFeatureType(types));

  auto const detailText = strings::JoinStrings(details, " • ");
  if (!detailText.empty())
    cell.detailTextLabel.text = @(detailText.c_str());
  else
    cell.detailTextLabel.text = nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
  if (!cell) {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle
                                  reuseIdentifier:@"BookmarksVCBookmarkItemCell"];
  }

  auto const bmId = [self markIdForRow:row];
  auto const &bm = GetFramework().GetBookmarkManager();
  Bookmark const *bookmark = bm.GetBookmark(bmId);
  cell.textLabel.text = @(bookmark->GetPreferredName().c_str());
  cell.imageView.image = ios_bookmark_ui_helper::ImageForBookmark(bookmark->GetColor(), bookmark->GetData().m_icon);

  CLLocation *lastLocation = [MWMLocationManager lastLocation];

  [self fillCell:cell withBookmarkDetails:bookmark andLocation:lastLocation];
  return cell;
}

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(CLLocation *)location {
  auto const bmId = [self markIdForRow:row];
  auto const &bm = GetFramework().GetBookmarkManager();
  Bookmark const *bookmark = bm.GetBookmark(bmId);
  if (!bookmark)
    return;
  [self fillCell:cell withBookmarkDetails:bookmark andLocation:location];
}

- (void)didSelectRow:(NSInteger)row {
  auto const bmId = [self markIdForRow:row];
  [Statistics logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
  
  auto const bookmark = GetFramework().GetBookmarkManager().GetBookmark(bmId);
  if (bookmark != nullptr && [[MWMBookmarksManager sharedManager] isGuide:bookmark->GetGroupId()]) {
    [Statistics logEvent:kStatGuidesBookmarkSelect
          withParameters:@{kStatServerId: [[MWMBookmarksManager sharedManager] getServerId:bookmark->GetGroupId()]}
             withChannel:StatisticsChannelRealtime];
  }
  
  // Same as "Close".
  [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
  GetFramework().ShowBookmark(bmId);
}

- (void)deleteRow:(NSInteger)row {
  auto const bmId = [self markIdForRow:row];
  [[MWMBookmarksManager sharedManager] deleteBookmark:bmId];
  [self.markIds removeObjectAtIndex:row];
}

@end
