#import "BookmarksSection.h"
#import "CircleView.h"
#import "ColorPickerView.h"
#import "MWMBookmarksManager.h"
#import "MWMLocationHelpers.h"
#import "MWMSearchManager.h"
#include "Framework.h"

#include "geometry/distance_on_sphere.hpp"

NS_ASSUME_NONNULL_BEGIN

namespace {
CGFloat const kPinDiameter = 22.0f;
}  // namespace

@interface BookmarksSection () {
  NSString *m_title;
  NSMutableArray<NSNumber *> *m_markIds;
  BOOL m_isEditable;
}

@end

@implementation BookmarksSection

- (instancetype)initWithTitle:(nullable NSString *)title
                      markIds:(MWMMarkIDCollection)markIds
                   isEditable:(BOOL)isEditable {
  self = [super init];
  if (self) {
    m_title = title;
    m_markIds = markIds.mutableCopy;
    m_isEditable = isEditable;
  }
  return self;
}

- (kml::MarkId)getMarkIdForRow:(NSInteger)row {
  return static_cast<kml::MarkId>(m_markIds[row].unsignedLongLongValue);
}

- (NSInteger)numberOfRows {
  return [m_markIds count];
}

- (nullable NSString *)title {
  return m_title;
}

- (BOOL)canEdit {
  return m_isEditable;
}

- (void)fillCell:(UITableViewCell *)cell
  withBookmarkDetails:(Bookmark const *)bookmark
          andLocation:(CLLocation *)location {
  std::vector<std::string> details;

  if (location) {
    m2::PointD const pos = bookmark->GetPivot();
    double const meters = ms::DistanceOnEarth(location.coordinate.latitude, location.coordinate.longitude,
                                              MercatorBounds::YToLat(pos.y), MercatorBounds::XToLon(pos.x));
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

  auto const bmId = [self getMarkIdForRow:row];
  auto const &bm = GetFramework().GetBookmarkManager();
  Bookmark const *bookmark = bm.GetBookmark(bmId);
  cell.textLabel.text = @(bookmark->GetPreferredName().c_str());
  cell.imageView.image = [CircleView createCircleImageWith:kPinDiameter
                                                  andColor:[ColorPickerView getUIColor:bookmark->GetColor()]
                                              andImageName:@(DebugPrint(bookmark->GetData().m_icon).c_str())];

  CLLocation *lastLocation = [MWMLocationManager lastLocation];

  [self fillCell:cell withBookmarkDetails:bookmark andLocation:lastLocation];
  return cell;
}

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(CLLocation *)location {
  auto const bmId = [self getMarkIdForRow:row];
  auto const &bm = GetFramework().GetBookmarkManager();
  Bookmark const *bookmark = bm.GetBookmark(bmId);
  if (!bookmark)
    return;
  [self fillCell:cell withBookmarkDetails:bookmark andLocation:location];
}

- (BOOL)didSelectRow:(NSInteger)row {
  auto const bmId = [self getMarkIdForRow:row];
  [Statistics logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
  // Same as "Close".
  [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
  GetFramework().ShowBookmark(bmId);
  return YES;
}

- (void)deleteRow:(NSInteger)row {
  auto const bmId = [self getMarkIdForRow:row];
  [[MWMBookmarksManager sharedManager] deleteBookmark:bmId];
  [m_markIds removeObjectAtIndex:row];
}

@end

NS_ASSUME_NONNULL_END
