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

#define PINDIAMETER 18

#define EMPTY_SECTION -666

@interface BookmarksVC() <MWMLocationObserver, MWMCategoryInfoCellDelegate>
{
  int m_infoSection;
  int m_trackSection;
  int m_bookmarkSection;
  int m_numberOfSections;
}
@end

@implementation BookmarksVC

- (instancetype)initWithCategory:(MWMMarkGroupID)index
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  if (self)
  {
    m_categoryId = index;
    auto const & bmManager = GetFramework().GetBookmarkManager();
    self.title = @(bmManager.GetCategoryName(m_categoryId).c_str());
    [self calculateSections];
  }
  return self;
}

- (kml::MarkId)getBookmarkIdByRow:(NSInteger)row
{
  auto const & bmManager = GetFramework().GetBookmarkManager();
  auto const & bookmarkIds = bmManager.GetUserMarkIds(m_categoryId);
  ASSERT_LESS(row, bookmarkIds.size(), ());
  auto it = bookmarkIds.begin();
  std::advance(it, row);
  return *it;
}

- (kml::TrackId)getTrackIdByRow:(NSInteger)row
{
  auto const & bmManager = GetFramework().GetBookmarkManager();
  auto const & trackIds = bmManager.GetTrackIds(m_categoryId);
  ASSERT_LESS(row, trackIds.size(), ());
  auto it = trackIds.begin();
  std::advance(it, row);
  return *it;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return m_numberOfSections;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == m_infoSection)
    return 1;
  else if (section == m_trackSection)
    return GetFramework().GetBookmarkManager().GetTrackIds(m_categoryId).size();
  else if (section == m_bookmarkSection)
    return GetFramework().GetBookmarkManager().GetUserMarkIds(m_categoryId).size();
  else
    return 0;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section == m_infoSection)
    return L(@"placepage_place_description");
  if (section == m_trackSection)
    return L(@"tracks");
  if (section == m_bookmarkSection)
    return L(@"bookmarks");
  return nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Framework & fr = GetFramework();
  
  auto & bmManager = fr.GetBookmarkManager();
  if (!bmManager.HasBmCategory(m_categoryId))
    return nil;

  UITableViewCell * cell = nil;
  // First section, contains info about current set
  if (indexPath.section == m_infoSection)
  {
    cell = [tableView dequeueReusableCellWithCellClass:MWMCategoryInfoCell.class indexPath:indexPath];
    auto infoCell = (MWMCategoryInfoCell *)cell;
    auto const & categoryData = bmManager.GetCategoryData(m_categoryId);
    [infoCell updateWithCategoryData:categoryData delegate:self];
  }
  else if (indexPath.section == m_trackSection)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"TrackCell"];
    if (!cell)
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"TrackCell"];
    kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
    Track const * tr = bmManager.GetTrack(trackId);
    cell.textLabel.text = @(tr->GetName().c_str());
    string dist;
    if (measurement_utils::FormatDistance(tr->GetLengthMeters(), dist))
      //Change Length before release!!!
      cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ %@", L(@"length"), @(dist.c_str())];
    else
      cell.detailTextLabel.text = nil;
    const dp::Color c = tr->GetColor(0);
    cell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[UIColor colorWithRed:c.GetRed()/255.f green:c.GetGreen()/255.f
                                                                                                   blue:c.GetBlue()/255.f alpha:1.f]];
  }
  // Contains bookmarks list
  else if (indexPath.section == m_bookmarkSection)
  {
    UITableViewCell * bmCell = (UITableViewCell *)[tableView dequeueReusableCellWithIdentifier:@"BookmarksVCBookmarkItemCell"];
    if (!bmCell)
      bmCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"BookmarksVCBookmarkItemCell"];
    kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
    Bookmark const * bm = bmManager.GetBookmark(bmId);
    if (bm)
    {
      bmCell.textLabel.text = @(bm->GetPreferredName().c_str());
      bmCell.imageView.image = [CircleView createCircleImageWith:PINDIAMETER andColor:[ColorPickerView getUIColor:bm->GetColor()]];

      CLLocation * lastLocation = [MWMLocationManager lastLocation];
      if (lastLocation)
      {
        double north = location_helpers::headingToNorthRad([MWMLocationManager lastHeading]);
        string distance;
        double azimut = -1.0;
        fr.GetDistanceAndAzimut(bm->GetPivot(), lastLocation.coordinate.latitude,
                                lastLocation.coordinate.longitude, north, distance, azimut);

        bmCell.detailTextLabel.text = @(distance.c_str());
      }
      else
        bmCell.detailTextLabel.text = nil;
    }
    else
      ASSERT(false, ("NULL bookmark"));

    cell = bmCell;
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

  Framework & f = GetFramework();
  auto & bmManager = f.GetBookmarkManager();
  bool const categoryExists = bmManager.HasBmCategory(m_categoryId);
  ASSERT(categoryExists, ("Nonexistent category"));
  if (indexPath.section == m_trackSection)
  {
    if (categoryExists)
    {
      kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
      Track const * tr = bmManager.GetTrack(trackId);
      ASSERT(tr, ("NULL track"));
      if (tr)
      {
        f.ShowTrack(*tr);
        [self.navigationController popToRootViewControllerAnimated:YES];
      }
    }
  }
  else if (indexPath.section == m_bookmarkSection)
  {
    if (categoryExists)
    {
      kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
      Bookmark const * bm = bmManager.GetBookmark(bmId);
      ASSERT(bm, ("NULL bookmark"));
      if (bm)
      {
        [Statistics logEvent:kStatEventName(kStatBookmarks, kStatShowOnMap)];
        // Same as "Close".
        [MWMSearchManager manager].state = MWMSearchManagerStateHidden;
        f.ShowBookmark(bm);
        [self.navigationController popToRootViewControllerAnimated:YES];
      }
    }
  }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (!GetFramework().GetBookmarkManager().IsCategoryFromCatalog(m_categoryId) &&
      (indexPath.section == m_trackSection || indexPath.section == m_bookmarkSection))
    return YES;
  return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == m_trackSection || indexPath.section == m_bookmarkSection)
  {
    auto & bmManager = GetFramework().GetBookmarkManager();
    if (bmManager.HasBmCategory(m_categoryId))
    {
      if (editingStyle == UITableViewCellEditingStyleDelete)
      {
        if (indexPath.section == m_trackSection)
        {
          kml::TrackId const trackId = [self getTrackIdByRow:indexPath.row];
          bmManager.GetEditSession().DeleteTrack(trackId);
        }
        else
        {
          kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
          [MWMBookmarksManager deleteBookmark:bmId];
        }
      }
      size_t previousNumberOfSections  = m_numberOfSections;
      [self calculateSections];
      //We can delete the row with animation, if number of sections stay the same.
      if (previousNumberOfSections == m_numberOfSections)
        [self.tableView deleteRowsAtIndexPaths:@[ indexPath ] withRowAnimation:UITableViewRowAnimationFade];
      else
        [self.tableView reloadData];
      if (bmManager.GetUserMarkIds(m_categoryId).size() + bmManager.GetTrackIds(m_categoryId).size() == 0)
      {
        self.navigationItem.rightBarButtonItem = nil;
        [self setEditing:NO animated:YES];
      }
    }
  }
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
  UITableViewHeaderFooterView * header = (UITableViewHeaderFooterView *)view;
  header.textLabel.textColor = [UIColor blackSecondaryText];
  header.textLabel.font = [UIFont medium14];
}

#pragma mark - MWMCategoryInfoCellDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell
{
  [self.tableView beginUpdates];
  cell.expanded = YES;
  [self.tableView endUpdates];
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  // Refresh distance
  auto & bmManager = GetFramework().GetBookmarkManager();
  if (bmManager.HasBmCategory(m_categoryId))
  {
    UITableView * table = (UITableView *)self.view;
    [table.visibleCells enumerateObjectsUsingBlock:^(UITableViewCell * cell, NSUInteger idx, BOOL * stop)
    {
      NSIndexPath * indexPath = [table indexPathForCell:cell];
      if (indexPath.section == self->m_bookmarkSection)
      {
        kml::MarkId const bmId = [self getBookmarkIdByRow:indexPath.row];
        Bookmark const * bm = bmManager.GetBookmark(bmId);
        if (bm)
        {
          m2::PointD const center = bm->GetPivot();
          double const metres = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
                                                    MercatorBounds::YToLat(center.y), MercatorBounds::XToLon(center.x));
          cell.detailTextLabel.text = location_helpers::formattedDistance(metres);
        }
      }
    }];
  }
}

//*********** End of Location manager callbacks ********************
//******************************************************************

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.tableView registerWithCellClass:MWMCategoryInfoCell.class];
}

- (void)viewWillAppear:(BOOL)animated
{
  [MWMLocationManager addObserver:self];

  // Display Edit button only if table is not empty
  auto & bmManager = GetFramework().GetBookmarkManager();
  if (bmManager.HasBmCategory(m_categoryId) && !bmManager.IsCategoryFromCatalog(m_categoryId)
    && (bmManager.GetUserMarkIds(m_categoryId).size() + bmManager.GetTrackIds(m_categoryId).size()))
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
  int index = 0;
  auto & bmManager = GetFramework().GetBookmarkManager();
  m_infoSection = bmManager.IsCategoryFromCatalog(m_categoryId) ? index++ : EMPTY_SECTION;
  m_trackSection = bmManager.GetTrackIds(m_categoryId).size() ? index++ : EMPTY_SECTION;
  m_bookmarkSection = bmManager.GetUserMarkIds(m_categoryId).size() ? index ++ : EMPTY_SECTION;
  m_numberOfSections = index;
}

@end
