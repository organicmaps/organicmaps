#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMBasePlacePageView.h"
#import "MWMCircularProgress.h"
#import "MWMFrameworkListener.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMPlacePageViewManager.h"
#import "MWMStorage.h"
#import "MWMViewController.h"
#import "NSString+Categories.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

#include "map/place_page_info.hpp"

#include "3party/opening_hours/opening_hours.hpp"
#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

namespace
{
CGFloat const kDownloadProgressViewLeftOffset = 12.;
CGFloat const kDownloadProgressViewTopOffset = 10.;
CGFloat const kLabelsBetweenMainOffset = 8.;
CGFloat const kLabelsBetweenSmallOffset = 4.;
CGFloat const kBottomPlacePageOffset = 16.;
CGFloat const kLeftOffset = 16.;
CGFloat const kDefaultHeaderHeight = 16.;
CGFloat const kLabelsPadding = kLeftOffset * 2;
CGFloat const kDirectionArrowSide = 20.;
CGFloat const kOffsetFromLabelsToDistance = 8.;
CGFloat const kOffsetFromDistanceToArrow = 5.;
CGFloat const kMaximumWidth = 360.;

enum class PlacePageSection
{
  Bookmark,
  Metadata,
  Editing,
  Booking
};

vector<MWMPlacePageCellType> const kSectionBookmarkCellTypes {
  MWMPlacePageCellTypeBookmark
};

vector<MWMPlacePageCellType> const kSectionMetadataCellTypes {
    MWMPlacePageCellTypePostcode, MWMPlacePageCellTypePhoneNumber, MWMPlacePageCellTypeWebsite, MWMPlacePageCellTypeURL,
    MWMPlacePageCellTypeEmail, MWMPlacePageCellTypeOpenHours, MWMPlacePageCellTypeWiFi, MWMPlacePageCellTypeCoordinate
};

vector<MWMPlacePageCellType> const kSectionEditingCellTypes {
  MWMPlacePageCellTypeEditButton,
  MWMPlacePageCellTypeAddBusinessButton,
  MWMPlacePageCellTypeAddPlaceButton
};

vector<MWMPlacePageCellType> const kSectionBookingCellTypes {
  MWMPlacePageCellTypeBookingMore
};

using TCellTypesSectionMap = pair<vector<MWMPlacePageCellType>, PlacePageSection>;

vector<TCellTypesSectionMap> const kCellTypesSectionMap {
  {kSectionBookmarkCellTypes, PlacePageSection::Bookmark},
  {kSectionMetadataCellTypes, PlacePageSection::Metadata},
  {kSectionEditingCellTypes, PlacePageSection::Editing},
  {kSectionBookingCellTypes, PlacePageSection::Booking}
};

MWMPlacePageCellTypeValueMap const kCellType2ReuseIdentifier{
    {MWMPlacePageCellTypeWiFi, "PlacePageInfoCell"},
    {MWMPlacePageCellTypeCoordinate, "PlacePageInfoCell"},
    {MWMPlacePageCellTypePostcode, "PlacePageInfoCell"},
    {MWMPlacePageCellTypeURL, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeWebsite, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeEmail, "PlacePageLinkCell"},
    {MWMPlacePageCellTypePhoneNumber, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeOpenHours, "MWMPlacePageOpeningHoursCell"},
    {MWMPlacePageCellTypeBookmark, "PlacePageBookmarkCell"},
    {MWMPlacePageCellTypeEditButton, "MWMPlacePageButtonCell"},
    {MWMPlacePageCellTypeAddBusinessButton, "MWMPlacePageButtonCell"},
    {MWMPlacePageCellTypeAddPlaceButton, "MWMPlacePageButtonCell"},
    {MWMPlacePageCellTypeBookingMore, "MWMPlacePageButtonCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = kCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != kCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
}

CGFloat placePageWidth()
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  return IPAD ? kMaximumWidth : (size.width > size.height ? MIN(kMaximumWidth, size.height) : size.width);
}

enum class AttributePosition
{
  Title,
  ExternalTitle,
  Subtitle,
  Schedule,
  Address
};
} // namespace

using namespace storage;

@interface MWMBasePlacePageView ()<
    UITableViewDelegate, UITableViewDataSource, MWMPlacePageOpeningHoursCellProtocol,
    MWMPlacePageBookmarkDelegate, MWMCircularProgressProtocol, MWMFrameworkStorageObserver>
{
  vector<PlacePageSection> m_sections;
  map<PlacePageSection, vector<MWMPlacePageCellType>> m_cells;
  map<MWMPlacePageCellType, UITableViewCell *> m_offscreenCells;
}

@property (weak, nonatomic) MWMPlacePageEntity * entity;
@property (weak, nonatomic) IBOutlet MWMPlacePage * ownerPlacePage;

@property (nonatomic, readwrite) BOOL openingHoursCellExpanded;
@property (nonatomic) BOOL isBookmarkCellExpanded;

@property (nonatomic) MWMCircularProgress * mapDownloadProgress;

@end

@implementation MWMBasePlacePageView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.featureTable.delegate = self;
  self.featureTable.dataSource = self;
  self.featureTable.separatorColor = [UIColor blackDividers];

  for (auto const & type : kCellType2ReuseIdentifier)
  {
    NSString * identifier = @(type.second.c_str());
    [self.featureTable registerNib:[UINib nibWithNibName:identifier bundle:nil]
            forCellReuseIdentifier:identifier];
  }

  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;
}

- (void)configureWithEntity:(MWMPlacePageEntity *)entity
{
  self.entity = entity;
  self.isBookmarkCellExpanded = NO;
  [self configTable];
  [self configure];
}

- (void)configTable
{
  m_sections.clear();
  m_cells.clear();
  m_offscreenCells.clear();
  for (auto const cellSection : kCellTypesSectionMap)
  {
    for (auto const cellType : cellSection.first)
    {
      if (![self.entity getCellValue:cellType])
        continue;
      m_sections.push_back(cellSection.second);
      m_cells[cellSection.second].push_back(cellType);
    }
  }

  sort(m_sections.begin(), m_sections.end());
  m_sections.erase(unique(m_sections.begin(), m_sections.end()), m_sections.end());
}

- (void)configure
{
  MWMPlacePageEntity * entity = self.entity;
  if (entity.isBookmark)
  {
    if (![entity.bookmarkTitle isEqualToString:entity.title] && entity.bookmarkTitle.length > 0)
    {
      self.titleLabel.text = entity.bookmarkTitle;
      self.externalTitleLabel.text = entity.title;
    }
    else
    {
      self.titleLabel.text = entity.title;
      self.externalTitleLabel.text = @"";
    }
  }
  else
  {
    self.titleLabel.text = entity.title;
    self.externalTitleLabel.text = @"";
  }

  self.subtitleLabel.text = entity.subtitle;
  if (entity.subtitle)
  {
    NSMutableAttributedString * str = [[NSMutableAttributedString alloc] initWithString:entity.subtitle];
    auto const separatorRanges = [entity.subtitle rangesOfString:@(place_page::Info::kSubtitleSeparator)];
    if (!separatorRanges.empty())
    {
      for (auto const & r : separatorRanges)
        [str addAttributes:@{NSForegroundColorAttributeName : [UIColor blackHintText]} range:r];

    }

    auto const starsRanges = [entity.subtitle rangesOfString:@(place_page::Info::kStarSymbol)];
    if (!starsRanges.empty())
    {
      for (auto const & r : starsRanges)
        [str addAttributes:@{NSForegroundColorAttributeName : [UIColor yellow]} range:r];
    }

    self.subtitleLabel.attributedText = str;
  }

  BOOL const isMyPosition = entity.isMyPosition;
  self.addressLabel.text = entity.address;

  if (!entity.bookingOnlinePrice.length)
  {
    self.bookingPriceLabel.text = entity.bookingPrice;
    [entity onlinePricingWithCompletionBlock:^
    {
      self.bookingPriceLabel.text = entity.bookingOnlinePrice;
      [self setNeedsLayout];
      [UIView animateWithDuration:kDefaultAnimationDuration animations:^
      {
        [self layoutIfNeeded];
      }];
    }
    failure:^
    {
      //TODO(Vlad): Process error.
    }];
  }
  else
  {
    self.bookingPriceLabel.text = entity.bookingOnlinePrice;
  }

  self.bookingRatingLabel.text = entity.bookingRating;
  self.bookingView.hidden = !entity.bookingPrice.length && !entity.bookingRating.length;
  BOOL const isHeadingAvaible = [CLLocationManager headingAvailable];
  BOOL const noLocation = MapsAppDelegate.theApp.locationManager.isLocationPendingOrNoPosition;
  self.distanceLabel.hidden = noLocation || isMyPosition;
  BOOL const hideDirection = noLocation || isMyPosition || !isHeadingAvaible;
  self.directionArrow.hidden = hideDirection;
  self.directionButton.hidden = hideDirection;

  [self.featureTable reloadData];
  [self configureCurrentShedule];
  [self configureMapDownloader];
  [MWMFrameworkListener addObserver:self];
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

- (void)configureMapDownloader
{
  TCountryId const & countryId = self.entity.countryId;
  if (countryId == kInvalidCountryId)
  {
    self.downloadProgressView.hidden = YES;
  }
  else
  {
    self.downloadProgressView.hidden = NO;
    NodeAttrs nodeAttrs;
    GetFramework().Storage().GetNodeAttrs(countryId, nodeAttrs);
    MWMCircularProgress * progress = self.mapDownloadProgress;
    switch (nodeAttrs.m_status)
    {
      case NodeStatus::NotDownloaded:
      case NodeStatus::Partly:
      {
        MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
          MWMCircularProgressStateSelected};
        [progress setImage:[UIImage imageNamed:@"ic_download"] forStates:affectedStates];
        [progress setColoring:MWMButtonColoringBlue forStates:affectedStates];
        progress.state = MWMCircularProgressStateNormal;
        break;
      }
      case NodeStatus::Downloading:
      {
        auto const & prg = nodeAttrs.m_downloadingProgress;
        progress.progress = static_cast<CGFloat>(prg.first) / prg.second;
        break;
      }
      case NodeStatus::InQueue:
        progress.state = MWMCircularProgressStateSpinner;
        break;
      case NodeStatus::Undefined:
      case NodeStatus::Error:
        progress.state = MWMCircularProgressStateFailed;
        break;
      case NodeStatus::OnDisk:
      {
        self.downloadProgressView.hidden = YES;
        [self setNeedsLayout];
        [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ [self layoutIfNeeded]; }];
        break;
      }
      case NodeStatus::OnDiskOutOfDate:
      {
        MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
          MWMCircularProgressStateSelected};
        [progress setImage:[UIImage imageNamed:@"ic_update"] forStates:affectedStates];
        [progress setColoring:MWMButtonColoringOther forStates:affectedStates];
        progress.state = MWMCircularProgressStateNormal;
        break;
      }
    }
  }
}

- (void)configureCurrentShedule
{
  MWMPlacePageOpeningHoursCell * cell =
                          static_cast<MWMPlacePageOpeningHoursCell *>(m_offscreenCells[MWMPlacePageCellTypeOpenHours]);
  if (cell)
  {
    self.placeScheduleLabel.text = cell.isClosed ? L(@"closed_now") : L(@"editor_time_open");
    self.placeScheduleLabel.textColor = cell.isClosed ? [UIColor red] : [UIColor blackSecondaryText];
  }
  else
  {
    self.placeScheduleLabel.text = @"";
  }
}

#pragma mark - Layout

- (AttributePosition)distanceAttributePosition
{
  if (self.addressLabel.text.length)
    return AttributePosition::Address;
  else if (!self.addressLabel.text.length && self.placeScheduleLabel.text.length)
    return AttributePosition::Schedule;
  else if (!self.placeScheduleLabel.text.length && self.subtitleLabel.text.length)
    return AttributePosition::Subtitle;
  else if (self.externalTitleLabel.text.length)
    return AttributePosition::ExternalTitle;
  else
    return AttributePosition::Title;
}

- (void)setupLabelsWidthWithBoundedWidth:(CGFloat)bound distancePosition:(AttributePosition)position
{
  auto const defaultMaxWidth = placePageWidth() - kLabelsPadding;
  CGFloat const labelsMaxWidth = self.downloadProgressView.hidden ? defaultMaxWidth :
                                            defaultMaxWidth - 2 * kDownloadProgressViewLeftOffset - self.downloadProgressView.width;
  switch (position)
  {
  case AttributePosition::Title:
    self.titleLabel.width = labelsMaxWidth - bound;
    self.subtitleLabel.width = self.addressLabel.width = self.externalTitleLabel.width = self.placeScheduleLabel.width = 0;
    break;
  case AttributePosition::ExternalTitle:
    self.addressLabel.width = self.subtitleLabel.width = self.placeScheduleLabel.width = 0;
    self.externalTitleLabel.width = labelsMaxWidth - bound;
    self.titleLabel.width = self.titleLabel.text.length > 0 ? labelsMaxWidth : 0;
    break;
  case AttributePosition::Subtitle:
    self.addressLabel.width = self.placeScheduleLabel.width = 0;
    self.titleLabel.width = self.titleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.externalTitleLabel.width = self.externalTitleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.subtitleLabel.width = labelsMaxWidth - bound;
    break;
  case AttributePosition::Schedule:
    self.addressLabel.width = 0;
    self.titleLabel.width = self.titleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.externalTitleLabel.width = self.externalTitleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.subtitleLabel.width = self.subtitleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.placeScheduleLabel.width = labelsMaxWidth - bound;
    break;
  case AttributePosition::Address:
    self.titleLabel.width = self.titleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.subtitleLabel.width = self.subtitleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.externalTitleLabel.width = self.externalTitleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.placeScheduleLabel.width = self.placeScheduleLabel.text.length > 0 ? labelsMaxWidth : 0;
    self.addressLabel.width = labelsMaxWidth - bound;
    break;
  }

  self.externalTitleLabel.width = self.entity.isBookmark ? labelsMaxWidth : 0;

  CGFloat const bookingWidth = labelsMaxWidth / 2;
  self.bookingRatingLabel.width = bookingWidth;
  self.bookingPriceLabel.width = bookingWidth - kLabelsBetweenSmallOffset;
  self.bookingView.width = labelsMaxWidth;

  [self.titleLabel sizeToFit];
  [self.subtitleLabel sizeToFit];
  [self.addressLabel sizeToFit];
  [self.externalTitleLabel sizeToFit];
  [self.placeScheduleLabel sizeToFit];
  [self.bookingRatingLabel sizeToFit];
  [self.bookingPriceLabel sizeToFit];
}

- (void)setDistance:(NSString *)distance
{
  self.distanceLabel.text = distance;
  self.distanceLabel.width = placePageWidth() - kLabelsPadding;
  [self.distanceLabel sizeToFit];
  [self layoutDistanceBoxWithPosition:[self distanceAttributePosition]];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  CGFloat const bound = self.distanceLabel.width + kDirectionArrowSide + kOffsetFromDistanceToArrow + kOffsetFromLabelsToDistance;
  AttributePosition const position = [self distanceAttributePosition];
  [self setupLabelsWidthWithBoundedWidth:bound distancePosition:position];
  [self setupBookingView];
  [self layoutLabelsAndBooking];
  [self layoutTableViewWithPosition:position];
  [self setDistance:self.distanceLabel.text];
  self.height = self.featureTable.height + self.ppPreview.height;
}

- (void)setupBookingView
{
  self.bookingRatingLabel.origin = {};
  self.bookingPriceLabel.origin = {self.bookingView.width - self.bookingPriceLabel.width, 0};
  self.bookingSeparator.origin = {0, self.bookingRatingLabel.maxY + kLabelsBetweenMainOffset};
  self.bookingView.height = self.bookingSeparator.maxY + kLabelsBetweenMainOffset;
}

- (void)layoutLabelsAndBooking
{
  BOOL const isDownloadProgressViewHidden = self.downloadProgressView.hidden;
  if (!isDownloadProgressViewHidden)
    self.downloadProgressView.origin = {kDownloadProgressViewLeftOffset, kDownloadProgressViewTopOffset};

  CGFloat const leftOffset = isDownloadProgressViewHidden ? kLeftOffset : self.downloadProgressView.maxX + kDownloadProgressViewLeftOffset;

  auto originFrom = ^ CGPoint (UIView * v, BOOL isBigOffset)
  {
    CGFloat const offset = isBigOffset ? kLabelsBetweenMainOffset : kLabelsBetweenSmallOffset;
    if ([v isKindOfClass:[UILabel class]])
      return {leftOffset, (static_cast<UILabel *>(v).text.length == 0 ? v.minY : v.maxY) +
                          offset};
    return {leftOffset, v.hidden ? v.minY : v.maxY + offset};
  };

  self.titleLabel.origin = {leftOffset, kLabelsBetweenSmallOffset};
  self.externalTitleLabel.origin = originFrom(self.titleLabel, NO);
  self.subtitleLabel.origin = originFrom(self.externalTitleLabel, NO);
  self.placeScheduleLabel.origin = originFrom(self.subtitleLabel, NO);
  self.bookingView.origin = originFrom(self.placeScheduleLabel, NO);
  self.addressLabel.origin = originFrom(self.bookingView, YES);
}

- (void)layoutDistanceBoxWithPosition:(AttributePosition)position
{
  auto getY = ^ CGFloat (AttributePosition p)
  {
    // Have to align distance box for the first label's line.
    CGFloat const defaultCenter = p == AttributePosition::Title ? 12 : 8;
    switch (position)
    {
    case AttributePosition::Title:
      return self.titleLabel.minY + defaultCenter;
    case AttributePosition::ExternalTitle:
      return self.externalTitleLabel.minY + defaultCenter;
    case AttributePosition::Subtitle:
      return self.subtitleLabel.minY + defaultCenter;
    case AttributePosition::Schedule:
      return self.placeScheduleLabel.minY + defaultCenter;
    case AttributePosition::Address:
      return self.addressLabel.minY + defaultCenter;
    }
  };

  CGFloat const distanceX = placePageWidth() - kLeftOffset - self.distanceLabel.width;
  CGFloat const directionX = distanceX - kOffsetFromDistanceToArrow - kDirectionArrowSide;
  CGFloat const y = getY(position);
  CGPoint const center = {directionX + kDirectionArrowSide / 2, y};
  self.directionArrow.center = center;
  self.directionButton.origin = {center.x - self.directionButton.width / 2, center.y - self.directionButton.height / 2};
  self.distanceLabel.center = {distanceX + self.distanceLabel.width / 2, self.directionArrow.center.y};
}

- (void)layoutTableViewWithPosition:(AttributePosition)position
{
  auto getY = ^ CGFloat (AttributePosition p)
  {
    if (self.bookingView.hidden)
    {
      switch (position)
      {
      case AttributePosition::Title:
        return self.titleLabel.maxY + kBottomPlacePageOffset;
      case AttributePosition::ExternalTitle:
        return self.externalTitleLabel.maxY + kBottomPlacePageOffset;
      case AttributePosition::Subtitle:
        return self.subtitleLabel.maxY + kBottomPlacePageOffset;
      case AttributePosition::Schedule:
        return self.placeScheduleLabel.maxY + kBottomPlacePageOffset;
      case AttributePosition::Address:
        return self.addressLabel.maxY + kBottomPlacePageOffset;
      }
    }
    else
    {
      switch (position)
      {
      case AttributePosition::Title:
      case AttributePosition::ExternalTitle:
      case AttributePosition::Subtitle:
      case AttributePosition::Schedule:
        return self.bookingView.maxY + kBottomPlacePageOffset;
      case AttributePosition::Address:
        return self.addressLabel.maxY + kBottomPlacePageOffset;
      }
    }
  };

  self.separatorView.minY = getY(position);
  self.ppPreview.frame = {{}, {self.ppPreview.superview.width, self.separatorView.maxY}};
  self.featureTable.minY = self.separatorView.maxY;
  self.featureTable.height = self.featureTable.contentSize.height;
}

#pragma mark - Actions

- (void)addBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatToggleBookmark)
                   withParameters:@{kStatValue : kStatAdd}];
  [self.subtitleLabel sizeToFit];

  m_sections.push_back(PlacePageSection::Bookmark);
  m_cells[PlacePageSection::Bookmark].push_back(MWMPlacePageCellTypeBookmark);
  sort(m_sections.begin(), m_sections.end());
  [self configure];
}

- (void)removeBookmark
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatToggleBookmark)
                   withParameters:@{kStatValue : kStatRemove}];

  auto const it = find(m_sections.begin(), m_sections.end(), PlacePageSection::Bookmark);
  if (it != m_sections.end())
  {
    m_sections.erase(it);
    m_cells.erase(PlacePageSection::Bookmark);
  }

  m_offscreenCells.erase(MWMPlacePageCellTypeBookmark);

  [self configure];
}

- (void)reloadBookmarkCell
{
  MWMPlacePageCellType const type = MWMPlacePageCellTypeBookmark;
  [self fillCell:m_offscreenCells[type] withType:type];
  [self configure];
  [CATransaction begin];
  [CATransaction setCompletionBlock:^
  {
     [self setNeedsLayout];
     dispatch_async(dispatch_get_main_queue(), ^{ [self.ownerPlacePage refresh]; });
  }];
  [CATransaction commit];
}

- (IBAction)directionButtonTap
{
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatCompass)];
  [self.ownerPlacePage.manager showDirectionViewWithTitle:self.titleLabel.text type:self.subtitleLabel.text];
}

- (void)updateAndLayoutMyPositionSpeedAndAltitude:(NSString *)text
{
  self.subtitleLabel.text = text;
  [self setNeedsLayout];
}

#pragma mark - MWMPlacePageBookmarkDelegate

- (void)reloadBookmark
{
  [self reloadBookmarkCell];
}

- (void)editBookmarkTap
{
  [self.ownerPlacePage editBookmark];
}

- (void)moreTap
{
  self.isBookmarkCellExpanded = YES;
  [self reloadBookmarkCell];
}

#pragma mark - MWMPlacePageOpeningHoursCellProtocol

- (BOOL)forcedButton
{
  return NO;
}

- (BOOL)isPlaceholder
{
  return NO;
}

- (BOOL)isEditor
{
  return NO;
}

- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded
{
  _openingHoursCellExpanded = openingHoursCellExpanded;
  UITableView * tv = self.featureTable;
  MWMPlacePageCellType const type = MWMPlacePageCellTypeOpenHours;
  [self fillCell:m_offscreenCells[MWMPlacePageCellTypeOpenHours] withType:type];
  [tv beginUpdates];
  [CATransaction begin];
  [CATransaction setCompletionBlock:^
  {
    [self setNeedsLayout];
    dispatch_async(dispatch_get_main_queue(), ^{ [self.ownerPlacePage refresh]; });
  }];
  [tv endUpdates];
  [CATransaction commit];
}

- (UITableViewCell *)offscreenCellForCellType:(MWMPlacePageCellType)type
{
  UITableViewCell * cell = m_offscreenCells[type];
  if (!cell)
  {
    NSString * identifier = reuseIdentifier(type);
    cell = [[[NSBundle mainBundle] loadNibNamed:identifier owner:nil options:nil] firstObject];
    [self fillCell:cell withType:type];
    m_offscreenCells[type] = cell;
  }
  return cell;
}

#pragma mark - UITableView

- (MWMPlacePageCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  return [self cellsForSection:indexPath.section][indexPath.row];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  return reuseIdentifier(cellType);
}

- (void)fillCell:(UITableViewCell *)cell withType:(MWMPlacePageCellType)cellType
{
  MWMPlacePageEntity * entity = self.entity;
  switch (cellType)
  {
    case MWMPlacePageCellTypeBookmark:
      [static_cast<MWMPlacePageBookmarkCell *>(cell) configWithText:entity.bookmarkDescription
                                                           delegate:self
                                                     placePageWidth:placePageWidth()
                                                             isOpen:self.isBookmarkCellExpanded
                                                             isHtml:entity.isHTMLDescription];
      break;
    case MWMPlacePageCellTypeOpenHours:
      [(MWMPlacePageOpeningHoursCell *)cell configWithDelegate:self info:[entity getCellValue:cellType]];
      break;
    case MWMPlacePageCellTypeEditButton:
    case MWMPlacePageCellTypeAddBusinessButton:
    case MWMPlacePageCellTypeAddPlaceButton:
    case MWMPlacePageCellTypeBookingMore:
      [static_cast<MWMPlacePageButtonCell *>(cell) config:self.ownerPlacePage.manager forType:cellType];
      break;
    default:
    {
      MWMPlacePageInfoCell * tCell = (MWMPlacePageInfoCell *)cell;
      tCell.currentEntity = self.entity;
      [tCell configureWithType:cellType info:[entity getCellValue:cellType]];
      break;
    }
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  UITableViewCell * cell = [self offscreenCellForCellType:cellType];
  switch (cellType)
  {
    case MWMPlacePageCellTypeBookmark:
      return ((MWMPlacePageBookmarkCell *)cell).cellHeight;
    case MWMPlacePageCellTypeOpenHours:
      return ((MWMPlacePageOpeningHoursCell *)cell).cellHeight;
    default:
    {
      [cell setNeedsUpdateConstraints];
      [cell updateConstraintsIfNeeded];
      cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
      [cell setNeedsLayout];
      [cell layoutIfNeeded];
      CGSize const size = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
      return size.height;
    }
  }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self cellsForSection:section].size();
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return m_sections.size();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const type = [self cellTypeForIndexPath:indexPath];
  NSString * identifier = reuseIdentifier(type);
  UITableViewCell * cell = m_offscreenCells[type];
  if (!cell)
    cell = [tableView dequeueReusableCellWithIdentifier:identifier];

  return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return section == m_sections.size() - 1 ? kDefaultHeaderHeight : 0.;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  if (m_sections[section] == PlacePageSection::Bookmark)
    return 0.001;
  return kDefaultHeaderHeight;
}

- (vector<MWMPlacePageCellType>)cellsForSection:(NSInteger)section
{
  NSAssert(m_sections.size() > section, @"Invalid section");
  return m_cells[m_sections[section]];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
{
  if (countryId != self.entity.countryId)
    return;
  [self configureMapDownloader];
}

- (void)processCountry:(TCountryId const &)countryId progress:(MapFilesDownloader::TProgress const &)progress
{
  if (countryId != self.entity.countryId)
    return;
  self.mapDownloadProgress.progress = static_cast<CGFloat>(progress.first) / progress.second;
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  TCountryId const & countryId = self.entity.countryId;
  NodeAttrs nodeAttrs;
  GetFramework().Storage().GetNodeAttrs(countryId, nodeAttrs);
  MWMAlertViewController * avc = self.ownerPlacePage.manager.ownerViewController.alertController;
  switch (nodeAttrs.m_status)
  {
    case NodeStatus::NotDownloaded:
    case NodeStatus::Partly:
      [MWMStorage downloadNode:countryId alertController:avc onSuccess:nil];
      break;
    case NodeStatus::Undefined:
    case NodeStatus::Error:
      [MWMStorage retryDownloadNode:countryId];
      break;
    case NodeStatus::OnDiskOutOfDate:
      [MWMStorage updateNode:countryId alertController:avc];
      break;
    case NodeStatus::Downloading:
    case NodeStatus::InQueue:
      [MWMStorage cancelDownloadNode:countryId];
      break;
    case NodeStatus::OnDisk:
      break;
  }
}

#pragma mark - Properties

- (MWMCircularProgress *)mapDownloadProgress
{
  if (!_mapDownloadProgress)
  {
    _mapDownloadProgress = [MWMCircularProgress downloaderProgressForParentView:self.downloadProgressView];
    _mapDownloadProgress.delegate = self;
  }
  return _mapDownloadProgress;
}

@end
