#import "MWMSearchHotelsFilterViewController.h"
#import "MWMSearchFilterViewController_Protected.h"
#import "SwiftBridge.h"

#include "base/stl_helpers.hpp"

#include <array>
#include <vector>

namespace
{

std::array<ftypes::IsHotelChecker::Type, static_cast<size_t>(ftypes::IsHotelChecker::Type::Count)> const kTypes = {{
  ftypes::IsHotelChecker::Type::Hotel,
  ftypes::IsHotelChecker::Type::Apartment,
  ftypes::IsHotelChecker::Type::CampSite,
  ftypes::IsHotelChecker::Type::Chalet,
  ftypes::IsHotelChecker::Type::GuestHouse,
  ftypes::IsHotelChecker::Type::Hostel,
  ftypes::IsHotelChecker::Type::Motel,
  ftypes::IsHotelChecker::Type::Resort
}};

unsigned makeMask(std::vector<ftypes::IsHotelChecker::Type> const & items)
{
  unsigned mask = 0;
  for (auto const i : items)
    mask = mask | 1U << static_cast<unsigned>(i);

  return mask;
}

enum class Section
{
  Rating,
  PriceCategory,
  Type,
  Count
};

NSAttributedString * makeString(NSString * primaryText, NSDictionary * primaryAttrs,
                                NSString * secondaryText, NSDictionary * secondaryAttrs)
{
  auto str = [[NSMutableAttributedString alloc] initWithString:primaryText attributes:primaryAttrs];
  if (secondaryText.length != 0)
  {
    auto secText = [NSString stringWithFormat:@"\n%@", secondaryText];
    auto secStr = [[NSAttributedString alloc] initWithString:secText attributes:secondaryAttrs];
    [str appendAttributedString:secStr];
  }
  return str.copy;
}

void configButton(UIButton * button, NSString * primaryText, NSString * secondaryText)
{
  UIFont * regular17 = [UIFont regular17];
  UIFont * regular10 = [UIFont regular10];

  UIColor * white = [UIColor white];

  UIImage * linkBlueImage = [UIImage imageWithColor:[UIColor linkBlue]];

  [button setBackgroundImage:[UIImage imageWithColor:white] forState:UIControlStateNormal];
  [button setBackgroundImage:linkBlueImage forState:UIControlStateSelected];
  [button setBackgroundImage:linkBlueImage
                    forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary * primarySelected =
      @{NSFontAttributeName : regular17, NSForegroundColorAttributeName : white};
  NSDictionary * secondarySelected =
      @{NSFontAttributeName : regular10, NSForegroundColorAttributeName : white};
  NSAttributedString * titleSelected =
      makeString(primaryText, primarySelected, secondaryText, secondarySelected);
  [button setAttributedTitle:titleSelected forState:UIControlStateSelected];
  [button setAttributedTitle:titleSelected
                    forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary * primaryNormal = @{
    NSFontAttributeName : regular17,
    NSForegroundColorAttributeName : [UIColor blackPrimaryText]
  };
  NSDictionary * secondaryNormal = @{
    NSFontAttributeName : regular10,
    NSForegroundColorAttributeName : [UIColor blackSecondaryText]
  };
  NSAttributedString * titleNormal =
      makeString(primaryText, primaryNormal, secondaryText, secondaryNormal);
  [button setAttributedTitle:titleNormal forState:UIControlStateNormal];

  button.titleLabel.textAlignment = NSTextAlignmentCenter;
}
}  // namespace

@interface MWMSearchHotelsFilterViewController () <UICollectionViewDelegate, UICollectionViewDataSource>
{
  std::vector<ftypes::IsHotelChecker::Type> m_selectedTypes;
}

@property(nonatomic) MWMFilterRatingCell * rating;
@property(nonatomic) MWMFilterPriceCategoryCell * price;
@property(nonatomic) MWMFilterCollectionHolderCell * type;

@end

@implementation MWMSearchHotelsFilterViewController

+ (MWMSearchHotelsFilterViewController *)controller
{
  NSString * identifier = [self className];
  return static_cast<MWMSearchHotelsFilterViewController *>(
      [self controllerWithIdentifier:identifier]);
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.tableView.estimatedRowHeight = 48;
  self.tableView.rowHeight = UITableViewAutomaticDimension;
}

- (void)initialRatingConfig
{
  MWMFilterRatingCell * rating = self.rating;
  configButton(rating.any, L(@"booking_filters_rating_any"), nil);
  configButton(rating.good, L(@"7.0+"), L(@"booking_filters_ragting_good"));
  configButton(rating.veryGood, L(@"8.0+"), L(@"booking_filters_rating_very_good"));
  configButton(rating.excellent, L(@"9.0+"), L(@"booking_filters_rating_excellent"));
  rating.any.selected = YES;
  [self resetRating];
}

- (void)initialPriceCategoryConfig
{
  MWMFilterPriceCategoryCell * price = self.price;
  configButton(price.one, L(@"$"), nil);
  configButton(price.two, L(@"$$"), nil);
  configButton(price.three, L(@"$$$"), nil);
  price.one.selected = NO;
  price.two.selected = NO;
  price.three.selected = NO;
  [self resetPriceCategory];
}

- (void)initialTypeConfig
{
  [self.type config];
  [self resetTypes];
}

- (void)reset
{
  [self resetRating];
  [self resetPriceCategory];
  [self resetTypes];
}

- (void)resetRating
{
  MWMFilterRatingCell * rating = self.rating;  
  rating.any.selected = YES;
  rating.good.selected = NO;
  rating.veryGood.selected = NO;
  rating.excellent.selected = NO;
}

- (void)resetPriceCategory
{
  MWMFilterPriceCategoryCell * price = self.price;
  price.one.selected = NO;
  price.two.selected = NO;
  price.three.selected = NO;
}

- (void)resetTypes
{
  m_selectedTypes.clear();
  [self.type.collectionView reloadData];
}

- (shared_ptr<search::hotels_filter::Rule>)rules
{
  using namespace search::hotels_filter;
  MWMFilterRatingCell * rating = self.rating;
  shared_ptr<Rule> ratingRule;
  if (rating.good.selected)
    ratingRule = Ge<Rating>(7.0);
  else if (rating.veryGood.selected)
    ratingRule = Ge<Rating>(8.0);
  else if (rating.excellent.selected)
    ratingRule = Ge<Rating>(9.0);

  MWMFilterPriceCategoryCell * price = self.price;
  shared_ptr<Rule> priceRule;
  if (price.one.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(1));
  if (price.two.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(2));
  if (price.three.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(3));

  shared_ptr<Rule> typeRule;
  if (!m_selectedTypes.empty())
    typeRule = OneOf(makeMask(m_selectedTypes));

  if (!ratingRule && !priceRule && !typeRule)
    return nullptr;

  return And(And(ratingRule, priceRule), typeRule);
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return my::Key(Section::Count);
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  switch (static_cast<Section>(indexPath.section))
  {
  case Section::Rating:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterRatingCell className] forIndexPath:indexPath];
    if (!self.rating)
    {
      self.rating = static_cast<MWMFilterRatingCell *>(cell);
      [self initialRatingConfig];
    }
    break;
  case Section::PriceCategory:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterPriceCategoryCell className] forIndexPath:indexPath];
    if (!self.price)
    {
      self.price = static_cast<MWMFilterPriceCategoryCell *>(cell);
      [self initialPriceCategoryConfig];
    }
    break;
  case Section::Type:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterCollectionHolderCell className] forIndexPath:indexPath];
    if (!self.type)
    {
      self.type = static_cast<MWMFilterCollectionHolderCell *>(cell);
      [self initialTypeConfig];
    }
    break;
  case Section::Count:
    NSAssert(false, @"Incorrect section!");
    break;
  }
  return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (static_cast<Section>(section))
  {
  case Section::Rating: return L(@"booking_filters_rating");
  case Section::PriceCategory: return L(@"booking_filters_price_category");
  case Section::Type: return L(@"type");
  default: return nil;
  }
}

#pragma mark - UICollectionViewDelegate & UICollectionViewDataSource

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  MWMFilterTypeCell * cell = [collectionView dequeueReusableCellWithReuseIdentifier:[MWMFilterTypeCell className]
                                                                       forIndexPath:indexPath];
  auto const type = kTypes[indexPath.row];
  cell.tagName.text = @(ftypes::IsHotelChecker::GetHotelTypeTag(type));
  cell.selected = find(m_selectedTypes.begin(), m_selectedTypes.end(), type) != m_selectedTypes.end();
  return cell;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
  return kTypes.size();
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
  auto const type = kTypes[indexPath.row];
  m_selectedTypes.emplace_back(type);
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
  auto const type = kTypes[indexPath.row];
  m_selectedTypes.erase(remove(m_selectedTypes.begin(), m_selectedTypes.end(), type));
}

@end
