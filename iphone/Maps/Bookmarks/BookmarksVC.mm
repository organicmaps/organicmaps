#import "BookmarksVC.h"
#import "CircleView.h"
#import "ColorPickerView.h"
#import "MWMBookmarksManager.h"
#import "MWMLocationHelpers.h"
#import "MWMLocationObserver.h"
#import "MWMSearchManager.h"
#import "MWMCategoryInfoCell.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "geometry/distance_on_sphere.hpp"

#include "coding/zip_creator.hpp"
#include "coding/internal/file_data.hpp"

#include <iterator>
#include <string>
#include <vector>

using namespace std;

namespace
{
enum class Section
{
  Info,
  Track,
  Bookmark
};

CGFloat const kPinDiameter = 18.0f;
}  // namespace

@interface BookmarksVC() <MWMLocationObserver, MWMCategoryInfoCellDelegate>
{
  vector<Section> m_sections;
}

@property(nonatomic) BOOL infoExpanded;

@end

@implementation BookmarksVC

- (instancetype)initWithCategory:(MWMMarkGroupID)index
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_categoryId = index;
    auto const & bm = GetFramework().GetBookmarkManager();
    self.title = @(bm.GetCategoryName(m_categoryId).c_str());
    [self calculateSections];
  }
  return self;
}

- (kml::MarkId)getBookmarkIdByRow:(NSInteger)row
{
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & bookmarkIds = bm.GetUserMarkIds(m_categoryId);
  ASSERT_LESS(row, bookmarkIds.size(), ());
  auto it = bookmarkIds.begin();
  advance(it, row);
  return *it;
}

- (kml::TrackId)getTrackIdByRow:(NSInteger)row
{
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & trackIds = bm.GetTrackIds(m_categoryId);
  ASSERT_LESS(row, trackIds.size(), ());
  auto it = trackIds.begin();
  advance(it, row);
  return *it;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return m_sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  auto const & bm = GetFramework().GetBookmarkManager();
  switch (m_sections.at(section))
  {
  case Section::Info:
    return 1;
  case Section::Track:
    return bm.GetTrackIds(m_categoryId).size();
  case Section::Bookmark:
    return bm.GetUserMarkIds(m_categoryId).size();
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (m_sections.at(section))
  {
  case Section::Info:
    return L(@"placepage_place_description");
  case Section::Track:
    return L(@"tracks_title");
  case Section::Bookmark:
    return L(@"bookmarks");
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto & f = GetFramework();
  auto const & bm = f.GetBookmarkManager();
  if (!bm.HasBmCategory(m_categoryId))
    return nil;

  UITableViewCell * cell = nil;
  switch (m_sections.at(indexPath.section))
  {
  case Section::Info:
  {
    cell = [tableView dequeueReusableCellWithCellClass:MWMCategoryInfoCell.class indexPath:indexPath];
    auto infoCell = (MWMCategoryInfoCell *)cell;
    auto const & categoryData = bm.GetCategoryData(m_categoryId);
    [infoCell updateWithCategoryData:categoryData delegate:self];
    infoCell.expanded = self.infoExpanded;
    break;
  }
  case Section::Track:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];

    kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
    Track const * tr = bm.GetTrack(trackId);
    cell.textLabel.text = @(tr->GetName().c_str());
    string dist;
    if (measurement_utils::FormatDistance(tr->GetLengthMeters(), dist))
      //Change Length before release!!!
      cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"length"), @(dist.c_str())];
    else
      cell.detailTextLabel.text = nil;
    dp::Color const c = tr->GetColor(0);
    cell.imageView.image = [CircleView createCircleImageWith:kPinDiameter
                                                    andColor:[UIColor colorWithRed:c.GetRed()/255.f
                                                                             green:c.GetGreen()/255.f
                                                                              blue:c.GetBlue()/255.f
                                                                             alpha:1.f]];
    break;
  }
  case Section::Bookmark:
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle
                                    reuseIdentifier:@"BookmarksVCBookmarkItemCell"];

    kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
    Bookmark const * bookmark = bm.GetBookmark(bmId);
    CHECK(cell, ("NULL bookmark"));
    cell.textLabel.text = @(bookmark->GetPreferredName().c_str());
    cell.imageView.image = [CircleView createCircleImageWith:kPinDiameter
                                                    andColor:[ColorPickerView getUIColor:bookmark->GetColor()]];

    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    if (lastLocation)
    {
      double north = location_helpers::headingToNorthRad([MWMLocationManager lastHeading]);
      string distance;
      double azimut = -1.0;
      f.GetDistanceAndAzimut(bookmark->GetPivot(), lastLocation.coordinate.latitude,
                              lastLocation.coordinate.longitude, north, distance, azimut);

      cell.detailTextLabel.text = @(distance.c_str());
    }
    else
    {
      cell.detailTextLabel.text = nil;
    }

    break;
  }
  }

  cell.backgroundColor = [UIColor white];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  cell.detailTextLabel.textColor = [UIColor blackSecondaryText];
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  auto & f = GetFramework();
  auto & bm = f.GetBookmarkManager();
  bool const categoryExists = bm.HasBmCategory(m_categoryId);
  CHECK(categoryExists, ("Nonexistent category"));
  switch (m_sections.at(indexPath.section))
  {
  case Section::Info:
    break;
  case Section::Track:
  {
    kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
    Track const * tr = bm.GetTrack(trackId);
    CHECK(tr, ("NULL track"));
    f.ShowTrack(*tr);
    [self.navigationController popToRootViewControllerAnimated:YES];
    break;
  }
  case Section::Bookmark:
  {
    kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
    Bookmark const * bookmark = bm.GetBookmark(bmId);
    CHECK(bookmark, ("NULL bookmark"));
    [Statistics logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
    // Same as "Close".
    [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
    f.ShowBookmark(bookmark);
    [self.navigationController popToRootViewControllerAnimated:YES];
    break;
  }
  }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const s = m_sections.at(indexPath.section);
  return !GetFramework().GetBookmarkManager().IsCategoryFromCatalog(m_categoryId) && s != Section::Info;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto const s = m_sections.at(indexPath.section);
  if (s == Section::Info)
    return;

  auto & bm = GetFramework().GetBookmarkManager();
  if (!bm.HasBmCategory(m_categoryId))
    return;

  if (editingStyle == UITableViewCellEditingStyleDelete)
  {
    if (s == Section::Track)
    {
      kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
      bm.GetEditSession().DeleteTrack(trackId);
    }
    else
    {
      kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
      [MWMBookmarksManager deleteBookmark:bmId];
    }
  }

  auto const previousNumberOfSections = m_sections.size();
  [self calculateSections];

  //We can delete the row with animation, if number of sections stay the same.
  if (previousNumberOfSections == m_sections.size())
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
  else
    [self.tableView reloadData];

  if (bm.GetUserMarkIds(m_categoryId).size() + bm.GetTrackIds(m_categoryId).size() == 0)
  {
    self.navigationItem.rightBarButtonItem = nil;
    [self setEditing:NO animated:YES];
  }
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
  auto header = (UITableViewHeaderFooterView *)view;
  header.textLabel.textColor = [UIColor blackSecondaryText];
  header.textLabel.font = [UIFont medium14];
}

#pragma mark - MWMCategoryInfoCellDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell
{
  [self.tableView beginUpdates];
  cell.expanded = YES;
  [self.tableView endUpdates];
  self.infoExpanded = YES;
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // Refresh distance
  auto const & bm = GetFramework().GetBookmarkManager();
  if (!bm.HasBmCategory(m_categoryId))
    return;

  auto table = (UITableView *)self.view;
  [table.visibleCells enumerateObjectsUsingBlock:^(UITableViewCell * cell, NSUInteger idx, BOOL * stop)
  {
    auto indexPath = [table indexPathForCell:cell];
    auto const s = self->m_sections.at(indexPath.section);
    if (s != Section::Bookmark)
      return;

    kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
    Bookmark const * bookmark = bm.GetBookmark(bmId);
    if (!bookmark)
      return;

    m2::PointD const center = bookmark->GetPivot();
    double const metres = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
                                              MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
    cell.detailTextLabel.text = location_helpers::formattedDistance(metres);
  }];
}

//*********** End of Location manager callbacks ********************
//******************************************************************

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.estimatedRowHeight = 44;
  [self.tableView registerWithCellClass:MWMCategoryInfoCell.class];
}

- (void)viewWillAppear:(BOOL)animated
{
  [MWMLocationManager addObserver:self];

  // Display Edit button only if table is not empty
  auto & bm = GetFramework().GetBookmarkManager();
  if (bm.HasBmCategory(m_categoryId) && !bm.IsCategoryFromCatalog(m_categoryId)
    && (bm.GetUserMarkIds(m_categoryId).size() + bm.GetTrackIds(m_categoryId).size()))
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
  else
    self.navigationItem.rightBarButtonItem = nil;

  [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [MWMLocationManager removeObserver:self];

  // Save possibly edited set name
  [super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
  // Disable all notifications in BM on appearance of this view.
  // It allows to significantly improve performance in case of bookmarks
  // modification. All notifications will be sent on controller's disappearance.
  [MWMBookmarksManager setNotificationsEnabled: NO];
  
  [super viewDidAppear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
  // Allow to send all notifications in BM.
  [MWMBookmarksManager setNotificationsEnabled: YES];
  
  [super viewDidDisappear:animated];
}

- (NSString *)categoryFileName
{
  return @(GetFramework().GetBookmarkManager().GetCategoryFileName(m_categoryId).c_str());
}

- (void)calculateSections
{
  m_sections.clear();
  auto const & bm = GetFramework().GetBookmarkManager();
  if (bm.IsCategoryFromCatalog(m_categoryId))
    m_sections.emplace_back(Section::Info);

  if (bm.GetTrackIds(m_categoryId).size() > 0)
    m_sections.emplace_back(Section::Track);

  if (bm.GetUserMarkIds(m_categoryId).size() > 0)
    m_sections.emplace_back(Section::Bookmark);
}

@end
