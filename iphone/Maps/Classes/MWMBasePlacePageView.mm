#import "MWMBasePlacePageView.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageTypeDescription.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"

static NSString * const kPlacePageLinkCellIdentifier = @"PlacePageLinkCell";
static NSString * const kPlacePageInfoCellIdentifier = @"PlacePageInfoCell";
static NSString * const kPlacePageBookmarkCellIdentifier = @"PlacePageBookmarkCell";

static CGFloat const kPlacePageTitleKoefficient = 0.63;
static CGFloat const kLeftOffset = 16.;
static CGFloat const kDirectionArrowSide = 26.;
static CGFloat const kOffsetFromTitleToDistance = 12.;
static CGFloat const kOffsetFromDistanceToArrow = 8.;
extern CGFloat const kBasePlacePageViewTitleBottomOffset = 2.;

@interface MWMBasePlacePageView ()

@property (weak, nonatomic) MWMPlacePageEntity * entity;
@property (weak, nonatomic) IBOutlet MWMPlacePage * ownerPlacePage;
@property (nonatomic) MWMPlacePageBookmarkCell * bookmarkSizingCell;

@end

@interface MWMBasePlacePageView (UITableView) <UITableViewDelegate, UITableViewDataSource>
@end

@implementation MWMBasePlacePageView

- (void)awakeFromNib
{
  [super awakeFromNib];

  self.featureTable.delegate = self;
  self.featureTable.dataSource = self;

  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageInfoCellIdentifier bundle:nil]
          forCellReuseIdentifier:kPlacePageInfoCellIdentifier];
  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageLinkCellIdentifier bundle:nil]
          forCellReuseIdentifier:kPlacePageLinkCellIdentifier];
  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageBookmarkCellIdentifier bundle:nil]
          forCellReuseIdentifier:kPlacePageBookmarkCellIdentifier];
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
  [self.entity insertBookmarkInTypes];
  [self configure];
}

- (void)removeBookmark
{
  [[Statistics instance] logEvent:kStatEventName(kStatPlacePage, kStatToggleBookmark)
                   withParameters:@{kStatValue : kStatRemove}];
  self.entity.type = MWMPlacePageEntityTypeRegular;
  [self.entity removeBookmarkFromTypes];
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

- (MWMPlacePageBookmarkCell *)bookmarkSizingCell
{
  if (!_bookmarkSizingCell)
    _bookmarkSizingCell = [self.featureTable dequeueReusableCellWithIdentifier:kPlacePageBookmarkCellIdentifier];
  return _bookmarkSizingCell;
}

@end

@implementation MWMBasePlacePageView (UITableView)

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSNumber * const currentType = self.entity.metadataTypes[indexPath.row];
  if (currentType.integerValue == MWMPlacePageMetadataTypeBookmark)
  {
    [self.bookmarkSizingCell config:self.ownerPlacePage forHeight:YES];
    CGFloat height = self.bookmarkSizingCell.cellHeight;
    return height;
  }

  CGFloat const defaultCellHeight = 44.;
  CGFloat const defaultWidth = tableView.width;
  CGFloat const leftOffset = 40.;
  CGFloat const rightOffset = 22.;
  UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(0., 0., defaultWidth - leftOffset - rightOffset, 10.)];
  label.numberOfLines = 0;
  label.text = self.entity.metadataValues[indexPath.row];
  [label sizeToFit];
  CGFloat const defaultCellOffset = 24.;
  return MAX(label.height + defaultCellOffset, defaultCellHeight);
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.entity.metadataTypes.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageMetadataType currentType = (MWMPlacePageMetadataType)[self.entity.metadataTypes[indexPath.row] integerValue];
  
  if (currentType == MWMPlacePageMetadataTypeBookmark)
  {
    MWMPlacePageBookmarkCell * cell = (MWMPlacePageBookmarkCell *)[tableView dequeueReusableCellWithIdentifier:kPlacePageBookmarkCellIdentifier];

    [cell config:self.ownerPlacePage forHeight:NO];
    return cell;
  }

  BOOL const isLinkTypeCell = (currentType == MWMPlacePageMetadataTypePhoneNumber || currentType == MWMPlacePageMetadataTypeEmail || currentType == MWMPlacePageMetadataTypeWebsite || currentType == MWMPlacePageMetadataTypeURL);
  NSString * const cellIdentifier =  isLinkTypeCell ? kPlacePageLinkCellIdentifier : kPlacePageInfoCellIdentifier;

  MWMPlacePageInfoCell * cell = (MWMPlacePageInfoCell *)[tableView dequeueReusableCellWithIdentifier:cellIdentifier];

  cell.currentEntity = self.entity;
  [cell configureWithType:currentType info:self.entity.metadataValues[indexPath.row]];
  return cell;
}

@end
