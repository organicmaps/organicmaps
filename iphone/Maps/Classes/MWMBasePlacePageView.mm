//
//  MWMBasePlagePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMBasePlacePageView.h"
#import "UIKitCategories.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageViewManager.h"
#import "MWMDirectionView.h"
#import "MWMPlacePageTypeDescription.h"
#import <CoreLocation/CoreLocation.h>

static NSString * const kPlacePageLinkCellIdentifier = @"PlacePageLinkCell";
static NSString * const kPlacePageInfoCellIdentifier = @"PlacePageInfoCell";
static NSString * const kPlacePageBookmarkCellIdentifier = @"PlacePageBookmarkCell";
extern CGFloat const kBookmarkCellHeight = 135.;

@interface MWMBasePlacePageView ()

@property (weak, nonatomic) MWMPlacePageEntity * entity;
@property (weak, nonatomic) IBOutlet MWMPlacePage * ownerPlacePage;

@end

@interface MWMBasePlacePageView (UITableView) <UITableViewDelegate, UITableViewDataSource>
@end

@implementation MWMBasePlacePageView

- (void)awakeFromNib
{
  [super awakeFromNib];
  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageInfoCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageInfoCellIdentifier];
  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageLinkCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageLinkCellIdentifier];
  [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageBookmarkCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageBookmarkCellIdentifier];
}

- (void)configureWithEntity:(MWMPlacePageEntity *)entity
{
  self.entity = entity;
  [self configure];
}

- (void)configure
{
  MWMPlacePageEntity const * entity = self.entity;
  MWMPlacePageEntityType type = entity.type;
  self.directionArrow.autoresizingMask = UIViewAutoresizingNone;

  if (type == MWMPlacePageEntityTypeBookmark)
  {
    self.titleLabel.text = entity.bookmarkTitle.length > 0 ? entity.bookmarkTitle : entity.title;
    self.typeLabel.text = entity.bookmarkCategory;
  }
  else
  {
    self.titleLabel.text = entity.title;
    self.typeLabel.text = entity.category;
  }

  BOOL const isMyPosition = type == MWMPlacePageEntityTypeMyPosition;
  BOOL const isHeadingAvaible = [CLLocationManager headingAvailable];
  self.distanceLabel.hidden = isMyPosition;
  self.directionArrow.hidden = isMyPosition || !isHeadingAvaible;
  self.directionButton.hidden = isMyPosition || !isHeadingAvaible;

  self.distanceLabel.text = @"";

  self.featureTable.delegate = self;
  self.featureTable.dataSource = self;
  [self.featureTable reloadData];
  [self layoutSubviews];
}

static CGFloat placePageWidth;
static CGFloat const kPlacePageTitleKoefficient = 0.6375f;
static CGFloat const leftOffset = 16.;
static CGFloat const directionArrowSide = 32.;
static CGFloat const offsetFromTitleToDistance = 12.;
static CGFloat const offsetFromDistanceToArrow = 8.;
static CGFloat const titleBottomOffset = 2.;

- (void)layoutSubviews
{
  [super layoutSubviews];
  MWMPlacePageEntity const * entity = self.entity;
  MWMPlacePageEntityType const type = entity.type;
  CGSize const size = [UIScreen mainScreen].bounds.size;
  CGFloat const maximumWidth = 360.;
  placePageWidth = size.width > size.height ? (size.height > maximumWidth ? maximumWidth : size.height) : size.width;
  CGFloat const maximumTitleWidth = kPlacePageTitleKoefficient * placePageWidth;
  CGFloat topOffset = (self.typeLabel.text.length > 0 || type == MWMPlacePageEntityTypeEle || type == MWMPlacePageEntityTypeHotel) ? 0 : 4.;
  CGFloat const typeBottomOffset = 10.;
  self.width = placePageWidth;
  self.titleLabel.width = maximumTitleWidth;
  [self.titleLabel sizeToFit];
  self.typeLabel.width = maximumTitleWidth;
  [self.typeLabel sizeToFit];
  CGFloat const typeMinY = self.titleLabel.maxY + titleBottomOffset;

  self.titleLabel.origin = CGPointMake(leftOffset, topOffset);
  self.typeLabel.origin = CGPointMake(leftOffset, typeMinY);

  [self layoutDistanceLabel];
  if (type == MWMPlacePageEntityTypeEle || type == MWMPlacePageEntityTypeHotel)
    [self layoutTypeDescription];

  CGFloat const typeHeight = self.typeLabel.text.length > 0 ? self.typeLabel.height : self.typeDescriptionView.height;
  self.featureTable.minY = typeMinY + typeHeight + typeBottomOffset;
  self.separatorView.minY = self.featureTable.minY - 1;
  self.featureTable.height = self.featureTable.contentSize.height;
  self.featureTable.contentOffset = CGPointZero;
  self.height = typeBottomOffset + titleBottomOffset + typeHeight + self.titleLabel.height + self.typeLabel.height + self.featureTable.height;
}

- (void)layoutTypeDescription
{
  MWMPlacePageEntity const * entity = self.entity;
  CGFloat const typeMinY = self.titleLabel.maxY + titleBottomOffset;
  [self.typeDescriptionView removeFromSuperview];
  self.typeDescriptionView = nil;
  MWMPlacePageTypeDescription * typeDescription = [[MWMPlacePageTypeDescription alloc] initWithPlacePageEntity:entity];
  self.typeDescriptionView = entity.type == MWMPlacePageEntityTypeHotel ? (UIView *)typeDescription.hotelDescription : (UIView *)typeDescription.eleDescription;
  self.typeDescriptionView.autoresizingMask = UIViewAutoresizingNone;
  BOOL const typeLabelIsNotEmpty = self.typeLabel.text.length > 0;
  CGFloat const minX = typeLabelIsNotEmpty ? self.typeLabel.minX + self.typeLabel.width + 2 *titleBottomOffset : leftOffset;
  CGFloat const minY = typeLabelIsNotEmpty ? self.typeLabel.center.y - self.typeDescriptionView.height / 2. - 1.: typeMinY;
  [self addSubview:self.typeDescriptionView];
  self.typeDescriptionView.origin = CGPointMake(minX, minY);
}

- (void)layoutDistanceLabel
{
  CGFloat const maximumTitleWidth = kPlacePageTitleKoefficient * placePageWidth;
  CGFloat const distanceLabelWidthPositionLeft = placePageWidth - maximumTitleWidth - directionArrowSide - 2 * leftOffset - offsetFromDistanceToArrow - offsetFromTitleToDistance;
  self.distanceLabel.width = distanceLabelWidthPositionLeft;
  [self.distanceLabel sizeToFit];
  CGFloat const titleCenterY = self.titleLabel.center.y;
  CGFloat const directionArrowMinX = placePageWidth - leftOffset - directionArrowSide;
  CGFloat const distanceLabelMinX = directionArrowMinX - self.distanceLabel.width - offsetFromDistanceToArrow;
  CGFloat const distanceLabelMinY = titleCenterY - self.distanceLabel.height / 2.;
  CGFloat const directionArrowMinY = titleCenterY - directionArrowSide / 2.;
  self.distanceLabel.origin = CGPointMake(distanceLabelMinX, distanceLabelMinY);
  self.directionArrow.center = CGPointMake(directionArrowMinX + directionArrowSide / 2., directionArrowMinY + directionArrowSide / 2.);
  self.directionButton.origin = self.directionArrow.origin;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event { }

- (void)addBookmark
{
  [self.typeDescriptionView removeFromSuperview];
  self.typeDescriptionView = nil;
  self.typeLabel.text = self.entity.bookmarkCategory;
  [self.typeLabel sizeToFit];
  NSUInteger const count = [self.entity.metadata[@"keys"] count];
  [self.entity.metadata[@"keys"] insertObject:@"Bookmark" atIndex:count];
  [self configure];
}

- (void)removeBookmark
{
  [self.entity.metadata[@"keys"] removeObject:@"Bookmark"];
  [self configure];
}

- (void)reloadBookmarkCell
{
  NSUInteger const count = [self.entity.metadata[@"keys"] count];
  NSIndexPath * index = [NSIndexPath indexPathForRow:count - 1 inSection:0];
  [self.featureTable reloadRowsAtIndexPaths:@[index] withRowAnimation:UITableViewRowAnimationAutomatic];
}

- (IBAction)directionButtonTap:(id)sender
{
  self.directionView = [MWMDirectionView directionViewForViewController:self.ownerPlacePage.manager.ownerViewController];
  self.directionView.titleLabel.text = self.titleLabel.text;
  self.directionView.typeLabel.text = self.typeLabel.text;
  self.directionView.distanceLabel.text = self.distanceLabel.text;
}

- (MWMDirectionView *)directionView
{
  if (_directionView)
    return _directionView;

  UIView * window = [[[UIApplication sharedApplication] delegate] window];
  __block MWMDirectionView * view = nil;

  [window.subviews enumerateObjectsUsingBlock:^(id subview, NSUInteger idx, BOOL *stop)
  {
    if ([subview isKindOfClass:[MWMDirectionView class]])
    {
      view = subview;
      *stop = YES;
    }
  }];

  return view;
}

@end

@implementation MWMBasePlacePageView (UITableView)

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * const currentKey = self.entity.metadata[@"keys"][indexPath.row];

  if ([currentKey isEqualToString:@"Bookmark"])
    return kBookmarkCellHeight;
  CGFloat const kDefaultCellHeight = 44.;
  CGFloat const defaultWidth = tableView.width;
  CGFloat const leftOffset = 40.;
  CGFloat const rightOffset = 22.;
  UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(0., 0., defaultWidth - leftOffset - rightOffset, 10.)];
  label.numberOfLines = 0;
  label.text = self.entity.metadata[@"values"][indexPath.row];
  [label sizeToFit];
  CGFloat const defaultCellOffset = 26.;
  return MAX(label.height + defaultCellOffset, kDefaultCellHeight);
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self.entity.metadata[@"keys"] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * const currentKey = self.entity.metadata[@"keys"][indexPath.row];

  if ([currentKey isEqualToString:@"Bookmark"])
  {
    MWMPlacePageBookmarkCell * cell = (MWMPlacePageBookmarkCell *)[tableView dequeueReusableCellWithIdentifier:kPlacePageBookmarkCellIdentifier];

    if (cell == nil)
      cell = [[MWMPlacePageBookmarkCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kPlacePageBookmarkCellIdentifier];

    cell.ownerTableView = tableView;
    cell.placePage = self.ownerPlacePage;
    [cell configure];
    return cell;
  }

  NSString * const kCellIdentifier = ([currentKey isEqualToString:@"PhoneNumber"] ||[currentKey isEqualToString:@"Email"] || [currentKey isEqualToString:@"Website"]) ? kPlacePageLinkCellIdentifier : kPlacePageInfoCellIdentifier;

  MWMPlacePageInfoCell * cell = (MWMPlacePageInfoCell *)[tableView dequeueReusableCellWithIdentifier:kCellIdentifier];

  if (cell == nil)
    cell = [[MWMPlacePageInfoCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellIdentifier];

  cell.currentEntity = self.entity;
  [cell configureWithIconTitle:currentKey info:self.entity.metadata[@"values"][indexPath.row]];
  return cell;
}

@end
