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
#import "UIColor+MapsMeColor.h"

extern CGFloat const kBottomPlacePageOffset = 15.;
extern CGFloat const kLabelsBetweenOffset = 8.;
namespace
{
CGFloat const kLeftOffset = 16.;
CGFloat const kLabelsPadding = kLeftOffset * 2;
CGFloat const kDirectionArrowSide = 20.;
CGFloat const kOffsetFromTitleToDistance = 8.;
CGFloat const kOffsetFromDistanceToArrow = 5.;
CGFloat const kMaximumWidth = 360.;

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

CGFloat placePageWidth()
{
  CGSize const size = UIScreen.mainScreen.bounds.size;
  return IPAD ? kMaximumWidth : (size.width > size.height ? MIN(kMaximumWidth, size.height) : size.width);
}

enum class AttributePosition
{
  Title,
  Type,
  Address
};
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
  self.featureTable.separatorColor = [UIColor blackDividers];
  for (auto const & type : gCellType2ReuseIdentifier)
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
  [self configure];
}

- (void)configure
{
  MWMPlacePageEntity * entity = self.entity;
  MWMPlacePageEntityType const type = entity.type;

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

  [self.typeDescriptionView removeFromSuperview];
  if (type == MWMPlacePageEntityTypeEle || type == MWMPlacePageEntityTypeHotel)
  {
    MWMPlacePageTypeDescription * description = [[MWMPlacePageTypeDescription alloc] initWithPlacePageEntity:entity];
    self.typeDescriptionView = static_cast<MWMPlacePageTypeDescriptionView *>(type == MWMPlacePageEntityTypeEle ?
                                                                              description.eleDescription :
                                                                              description.hotelDescription);
    [self addSubview:self.typeDescriptionView];
  }
  else
  {
    self.typeDescriptionView = nil;
  }

  self.addressLabel.text = entity.address;
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
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

#pragma mark - Layout

- (AttributePosition)distanceAttributePosition
{
  if ((self.typeLabel.text.length || self.typeDescriptionView))
    return AttributePosition::Type;
  else if ((!self.typeLabel.text.length && !self.typeDescriptionView) && self.addressLabel.text.length)
    return AttributePosition::Address;
  else
    return AttributePosition::Title;
}

- (void)setupLabelsWidthWithBoundedWidth:(CGFloat)bound distancePosition:(AttributePosition)position
{
  CGFloat const labelsMaxWidth = placePageWidth() - kLabelsPadding;
  switch (position)
  {
  case AttributePosition::Title:
    self.titleLabel.width = labelsMaxWidth - bound;
    self.typeLabel.width = self.addressLabel.width = 0;
    break;
  case AttributePosition::Type:
    if (self.addressLabel.text.length > 0)
    {
      self.titleLabel.width = self.addressLabel.width = labelsMaxWidth;
      self.typeLabel.width = labelsMaxWidth - bound;
    }
    else
    {
      self.titleLabel.width = labelsMaxWidth;
      self.typeLabel.width = labelsMaxWidth - bound;
      self.addressLabel.width = 0;
    }
    break;
  case AttributePosition::Address:
    self.titleLabel.width = labelsMaxWidth;
    self.typeLabel.width = 0;
    self.addressLabel.width = labelsMaxWidth - bound;
    break;
  }
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.distanceLabel.width = placePageWidth() - kLabelsPadding;
  [self.distanceLabel sizeToFit];
  CGFloat const bound = self.distanceLabel.width + kDirectionArrowSide + kOffsetFromDistanceToArrow + kOffsetFromTitleToDistance;
  AttributePosition const position = [self distanceAttributePosition];
  [self setupLabelsWidthWithBoundedWidth:bound distancePosition:position];
  [self.titleLabel sizeToFit];
  [self.typeLabel sizeToFit];
  [self.addressLabel sizeToFit];
  [self layoutLabels];
  [self.typeDescriptionView layoutNearPoint:{self.typeLabel.maxX, self.typeLabel.minY}];
  [self layoutDistanceBoxWithPosition:position];
  [self layoutTableViewWithPosition:position];
  self.height = self.featureTable.height + self.separatorView.height + self.titleLabel.height +
                (self.typeLabel.text.length > 0 ? self.typeLabel.height + kLabelsBetweenOffset : 0) +
                (self.addressLabel.text.length > 0 ? self.addressLabel.height + kLabelsBetweenOffset : 0) + kBottomPlacePageOffset;
}

- (void)layoutLabels
{
  self.titleLabel.origin = {kLeftOffset, 0};
  self.typeLabel.origin = {kLeftOffset, self.titleLabel.maxY + kLabelsBetweenOffset};
  self.addressLabel.origin = {kLeftOffset, self.typeLabel.maxY + kLabelsBetweenOffset};
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
    case AttributePosition::Type:
      return self.typeLabel.minY + defaultCenter;
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
    switch (position)
    {
    case AttributePosition::Title:
      return self.titleLabel.maxY + kBottomPlacePageOffset;
    case AttributePosition::Type:
      return (self.addressLabel.text.length > 0 ? self.addressLabel.maxY : self.typeLabel.maxY) + kBottomPlacePageOffset;
    case AttributePosition::Address:
      return self.addressLabel.maxY + kBottomPlacePageOffset;
    }
  };

  self.separatorView.minY = getY(position);
  self.featureTable.minY = self.separatorView.maxY;
  self.featureTable.height = self.featureTable.contentSize.height;
}

#pragma mark - Actions

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
      [(MWMPlacePageOpeningHoursCell *)cell configWithDelegate:self info:[entity getCellValue:cellType]];
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
