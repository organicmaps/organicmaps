#import "MWMBasePlacePageView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMPlacePageTypeDescription.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"

extern CGFloat const kBasePlacePageViewTitleBottomOffset = 2.;

namespace
{
CGFloat const kPlacePageTitleKoefficient = 0.63;
CGFloat const kLeftOffset = 16.;
CGFloat const kDirectionArrowSide = 26.;
CGFloat const kOffsetFromTitleToDistance = 12.;
CGFloat const kOffsetFromDistanceToArrow = 8.;

MWMPlacePageCellTypeValueMap const gCellType2ReuseIdentifier{
    {MWMPlacePageCellTypeWiFi, "PlacePageInfoCell"},
    {MWMPlacePageCellTypeCoordinate, "PlacePageInfoCell"},
    {MWMPlacePageCellTypePostcode, "PlacePageInfoCell"},
    {MWMPlacePageCellTypeURL, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeWebsite, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeEmail, "PlacePageLinkCell"},
    {MWMPlacePageCellTypePhoneNumber, "PlacePageLinkCell"},
    {MWMPlacePageCellTypeOpenHours, "MWMPlacePageOpeningHoursCell"},
    {MWMPlacePageCellTypeBookmark, "PlacePageBookmarkCell"},
    {MWMPlacePageCellTypeEditButton, "MWMPlacePageButtonCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = gCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != gCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
}
} // namespace

@interface MWMBasePlacePageView () <MWMPlacePageOpeningHoursCellProtocol>

@property (weak, nonatomic) MWMPlacePageEntity * entity;
@property (weak, nonatomic) IBOutlet MWMPlacePage * ownerPlacePage;

@property (nonatomic) NSMutableDictionary<NSString *, UITableViewCell *> * offscreenCells;

@property (nonatomic, readwrite) BOOL openingHoursCellExpanded;

@end

@interface MWMBasePlacePageView (UITableView) <UITableViewDelegate, UITableViewDataSource>
@end

@implementation MWMBasePlacePageView

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.featureTable.delegate = self;
  self.featureTable.dataSource = self;
  for (auto const & type : gCellType2ReuseIdentifier)
  {
    NSString * identifier = @(type.second.c_str());
    [self.featureTable registerNib:[UINib nibWithNibName:identifier bundle:nil]
            forCellReuseIdentifier:identifier];
  }
}

- (void)configureWithEntity:(MWMPlacePageEntity *)entity
{
  self.entity = entity;
  [self configure];
}

- (void)configure
{
  MWMPlacePageEntity * entity = self.entity;
  MWMPlacePageEntityType const type = entity.type;
  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;

  if (type == MWMPlacePageEntityTypeBookmark)
  {
    self.titleLabel.text = entity.bookmarkTitle.length > 0 ? entity.bookmarkTitle : entity.title;
    self.typeLabel.text = [entity.bookmarkCategory capitalizedString];
  }
  else
  {
    self.titleLabel.text = entity.title;
    self.typeLabel.text = [entity.category capitalizedString];
  }

  BOOL const isMyPosition = type == MWMPlacePageEntityTypeMyPosition;
  BOOL const isHeadingAvaible = [CLLocationManager headingAvailable];
  using namespace location;
  EMyPositionMode const mode = self.ownerPlacePage.manager.myPositionMode;
  BOOL const noLocation = (mode == EMyPositionMode::MODE_UNKNOWN_POSITION || mode == EMyPositionMode::MODE_PENDING_POSITION);
  self.distanceLabel.hidden = noLocation || isMyPosition;
  BOOL const hideDirection = noLocation || isMyPosition || !isHeadingAvaible;
  self.directionArrow.hidden = hideDirection;
  self.directionButton.hidden = hideDirection;

  [self.featureTable reloadData];
  [self layoutSubviews];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  MWMPlacePageEntity * entity = self.entity;
  MWMPlacePageEntityType const type = entity.type;
  CGFloat const maximumWidth = 360.;
  CGSize const size = [UIScreen mainScreen].bounds.size;
  CGFloat const placePageWidth =
      IPAD ? maximumWidth : size.width > size.height ? MIN(maximumWidth, size.height) : size.width;
  CGFloat const maximumTitleWidth = kPlacePageTitleKoefficient * placePageWidth;
  BOOL const isExtendedType =
      type == MWMPlacePageEntityTypeEle || type == MWMPlacePageEntityTypeHotel;
  CGFloat const topOffset = (self.typeLabel.text.length > 0 || isExtendedType) ? 0 : 4.;
  CGFloat const typeBottomOffset = 10.;
  self.width = placePageWidth;
  self.titleLabel.width = maximumTitleWidth;
  [self.titleLabel sizeToFit];
  self.typeLabel.width = maximumTitleWidth;
  [self.typeLabel sizeToFit];
  CGFloat const typeMinY = self.titleLabel.maxY + kBasePlacePageViewTitleBottomOffset;

  self.titleLabel.origin = CGPointMake(kLeftOffset, topOffset);
  self.typeLabel.origin = CGPointMake(kLeftOffset, typeMinY);

  [self.typeDescriptionView removeFromSuperview];
  if (isExtendedType)
    [self layoutTypeDescription];

  CGFloat const typeHeight =
      self.typeLabel.text.length > 0 ? self.typeLabel.height : self.typeDescriptionView.height;
  self.featureTable.minY = typeMinY + typeHeight + typeBottomOffset;
  self.separatorView.minY = self.featureTable.minY - 1;
  [self layoutDistanceLabelWithPlacePageWidth:placePageWidth];
  self.featureTable.height = self.featureTable.contentSize.height;
  self.height = typeBottomOffset + kBasePlacePageViewTitleBottomOffset + self.titleLabel.height +
                self.typeLabel.height + self.featureTable.height;
}

- (void)layoutTypeDescription
{
  MWMPlacePageEntity * entity = self.entity;
  CGFloat const typeMinY = self.titleLabel.maxY + kBasePlacePageViewTitleBottomOffset;
  MWMPlacePageTypeDescription * typeDescription =
      [[MWMPlacePageTypeDescription alloc] initWithPlacePageEntity:entity];
  self.typeDescriptionView = entity.type == MWMPlacePageEntityTypeHotel
                                 ? (UIView *)typeDescription.hotelDescription
                                 : (UIView *)typeDescription.eleDescription;
  self.typeDescriptionView.autoresizingMask = UIViewAutoresizingNone;
  BOOL const typeLabelIsNotEmpty = self.typeLabel.text.length > 0;
  CGFloat const minX =
      typeLabelIsNotEmpty
          ? self.typeLabel.minX + self.typeLabel.width + 2 * kBasePlacePageViewTitleBottomOffset
          : kLeftOffset;
  CGFloat const minY = typeLabelIsNotEmpty
                           ? self.typeLabel.center.y - self.typeDescriptionView.height / 2. - 1.
                           : typeMinY;
  if (![self.subviews containsObject:self.typeDescriptionView])
    [self addSubview:self.typeDescriptionView];

  self.typeDescriptionView.origin = CGPointMake(minX, minY);
}

- (void)layoutDistanceLabelWithPlacePageWidth:(CGFloat)placePageWidth
{
  CGFloat const maximumTitleWidth = kPlacePageTitleKoefficient * placePageWidth;
  CGFloat const distanceLabelWidthPositionLeft = placePageWidth - maximumTitleWidth - kDirectionArrowSide - 2 * kLeftOffset - kOffsetFromDistanceToArrow - kOffsetFromTitleToDistance;
  self.distanceLabel.width = distanceLabelWidthPositionLeft;
  [self.distanceLabel sizeToFit];
  CGFloat const directionArrowMinX = placePageWidth - kLeftOffset - kDirectionArrowSide;
  CGFloat const distanceLabelMinX = directionArrowMinX - self.distanceLabel.width - kOffsetFromDistanceToArrow;
  CGFloat const directionArrowCenterY = self.separatorView.maxY / 2.;
  self.directionArrow.center = CGPointMake(directionArrowMinX + kDirectionArrowSide / 2., directionArrowCenterY);
  self.distanceLabel.origin = CGPointMake(distanceLabelMinX, directionArrowCenterY - self.distanceLabel.height / 2.);
  self.directionButton.origin = self.directionArrow.origin;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
// Prevent super call to stop event propagation
// [super touchesBegan:touches withEvent:event];
}

- (void)addBookmark
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatToggleBookmark)
                   withParameters:@{kStatValue : kStatAdd}];
  self.entity.type = MWMPlacePageEntityTypeBookmark;
  [self.typeDescriptionView removeFromSuperview];
  self.typeDescriptionView = nil;
  [self.typeLabel sizeToFit];
  [self.entity addBookmarkField];
  [self configure];
}

- (void)removeBookmark
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatToggleBookmark)
                   withParameters:@{kStatValue : kStatRemove}];
  self.entity.type = MWMPlacePageEntityTypeRegular;
  [self.entity removeBookmarkField];
  [self configure];
}

- (void)reloadBookmarkCell
{
  [self configure];
  [self.featureTable reloadData];
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

- (IBAction)directionButtonTap
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatCompass)];
  [self.ownerPlacePage.manager showDirectionViewWithTitle:self.titleLabel.text type:self.typeLabel.text];
}

- (void)updateAndLayoutMyPositionSpeedAndAltitude:(NSString *)text
{
  self.typeLabel.text = text;
  [self setNeedsLayout];
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

- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded forCell:(UITableViewCell *)cell
{
  _openingHoursCellExpanded = openingHoursCellExpanded;
  UITableView * tv = self.featureTable;
  NSIndexPath * indexPath = [tv indexPathForCell:cell];
  [CATransaction begin];
  [tv beginUpdates];
  [CATransaction setCompletionBlock:^
  {
    [self setNeedsLayout];
    dispatch_async(dispatch_get_main_queue(), ^{ [self.ownerPlacePage refresh]; });
  }];
  [tv reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
  [tv endUpdates];
  [CATransaction commit];
}

- (void)editPlace
{
  [self.ownerPlacePage editPlace];
}

- (UITableViewCell *)offscreenCellForIdentifier:(NSString *)reuseIdentifier
{
  UITableViewCell * cell = self.offscreenCells[reuseIdentifier];
  if (!cell)
  {
    cell = [[[NSBundle mainBundle] loadNibNamed:reuseIdentifier owner:nil options:nil] firstObject];
    self.offscreenCells[reuseIdentifier] = cell;
  }
  return cell;
}

@end

@implementation MWMBasePlacePageView (UITableView)

- (MWMPlacePageCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const cellType = [self.entity getCellType:indexPath.row];
  return cellType;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  return reuseIdentifier(cellType);
}

- (void)fillCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath forHeight:(BOOL)forHeight
{
  MWMPlacePageEntity * entity = self.entity;
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeBookmark:
      [(MWMPlacePageBookmarkCell *)cell config:self.ownerPlacePage forHeight:NO];
      break;
    case MWMPlacePageCellTypeOpenHours:
      [(MWMPlacePageOpeningHoursCell *)cell configWithDelegate:self info:[entity getCellValue:cellType] lastCell:NO];
      break;
    case MWMPlacePageCellTypeEditButton:
      [(MWMPlacePageButtonCell *)cell config:self.ownerPlacePage];
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
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  UITableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  [self fillCell:cell atIndexPath:indexPath forHeight:YES];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
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

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(UITableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath forHeight:NO];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self.entity getCellsCount];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
}

@end
