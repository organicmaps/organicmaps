#import "BookmarksSection.h"
#import "CircleView.h"
#import "ColorPickerView.h"
#import "MWMBookmarksManager.h"
#import "MWMLocationHelpers.h"
#import "MWMSearchManager.h"
#include "Framework.h"

#include "geometry/distance_on_sphere.hpp"

namespace {
CGFloat const kPinDiameter = 22.0f;
}  // namespace

@interface BookmarksSection ()

@property(weak, nonatomic) id<BookmarksSectionDelegate> delegate;

@end

@implementation BookmarksSection

- (instancetype)initWithDelegate:(id<BookmarksSectionDelegate>)delegate {
  return [self initWithBlockIndex:nil delegate:delegate];
}

- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate:(id<BookmarksSectionDelegate>)delegate {
  self = [super init];
  if (self) {
    _blockIndex = blockIndex;
    _delegate = delegate;
  }
  return self;
}

- (NSInteger)numberOfRows {
  return [self.delegate numberOfBookmarksInSection:self];
}

- (NSString *)title {
  return [self.delegate titleOfBookmarksSection:self];
}

- (BOOL)canEdit {
  return [self.delegate canEditBookmarksSection:self];
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
  CHECK(cell, ("Invalid bookmark cell."));

  kml::MarkId const bmId = [self.delegate bookmarkSection:self getBookmarkIdByRow:row];
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

- (void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(location::GpsInfo const &)info {
  kml::MarkId const bmId = [self.delegate bookmarkSection:self getBookmarkIdByRow:row];
  auto const &bm = GetFramework().GetBookmarkManager();
  Bookmark const *bookmark = bm.GetBookmark(bmId);
  if (!bookmark)
    return;
  CLLocation *location = [[CLLocation alloc] initWithLatitude:info.m_latitude longitude:info.m_longitude];
  [self fillCell:cell withBookmarkDetails:bookmark andLocation:location];
}

- (BOOL)didSelectRow:(NSInteger)row {
  kml::MarkId const bmId = [self.delegate bookmarkSection:self getBookmarkIdByRow:row];
  [Statistics logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
  // Same as "Close".
  [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
  GetFramework().ShowBookmark(bmId);
  return YES;
}

- (BOOL)deleteRow:(NSInteger)row {
  kml::MarkId const bmId = [self.delegate bookmarkSection:self getBookmarkIdByRow:row];
  [[MWMBookmarksManager sharedManager] deleteBookmark:bmId];
  return [self.delegate bookmarkSection:self onDeleteBookmarkInRow:row];
}

@end
