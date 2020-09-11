#import "MWMSearchHotelsFilterViewController.h"

#import <CoreApi/MWMEye.h>

#import "MWMSearchFilterViewController_Protected.h"
#import "Statistics.h"
#import "SwiftBridge.h"

namespace {
static NSString *const kHotelTypePattern = @"search_hotel_filter_%@";

std::array<ftypes::IsHotelChecker::Type, base::Underlying(ftypes::IsHotelChecker::Type::Count)> const kTypes = {
  {ftypes::IsHotelChecker::Type::Hotel, ftypes::IsHotelChecker::Type::Apartment, ftypes::IsHotelChecker::Type::CampSite,
   ftypes::IsHotelChecker::Type::Chalet, ftypes::IsHotelChecker::Type::GuestHouse, ftypes::IsHotelChecker::Type::Hostel,
   ftypes::IsHotelChecker::Type::Motel, ftypes::IsHotelChecker::Type::Resort}};

enum class Section { Rating, PriceCategory, Type, Count };

NSAttributedString *makeString(NSString *primaryText, NSDictionary *primaryAttrs, NSString *secondaryText,
                               NSDictionary *secondaryAttrs) {
  auto str = [[NSMutableAttributedString alloc] initWithString:primaryText attributes:primaryAttrs];
  if (secondaryText.length != 0) {
    auto secText = [NSString stringWithFormat:@"\n%@", secondaryText];
    auto secStr = [[NSAttributedString alloc] initWithString:secText attributes:secondaryAttrs];
    [str appendAttributedString:secStr];
  }
  return str.copy;
}

void configButton(UIButton *button, NSString *primaryText, NSString *secondaryText) {
  UIFont *primaryFont = [UIFont regular14];
  UIFont *secondaryFont = [UIFont medium10];

  UIColor *white = [UIColor white];

  UIImage *linkBlueImage = [UIImage imageWithColor:[UIColor linkBlue]];
  UIImage *linkBlueHighlightedImage = [UIImage imageWithColor:[UIColor linkBlueHighlighted]];

  [button setBackgroundImage:[UIImage imageWithColor:white] forState:UIControlStateNormal];
  [button setBackgroundImage:linkBlueImage forState:UIControlStateSelected];
  [button setBackgroundImage:linkBlueHighlightedImage forState:UIControlStateHighlighted];
  [button setBackgroundImage:linkBlueHighlightedImage forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary *primarySelected = @{NSFontAttributeName: primaryFont, NSForegroundColorAttributeName: white};
  NSDictionary *secondarySelected = @{NSFontAttributeName: secondaryFont, NSForegroundColorAttributeName: white};
  NSAttributedString *titleSelected = makeString(primaryText, primarySelected, secondaryText, secondarySelected);
  [button setAttributedTitle:titleSelected forState:UIControlStateSelected];
  [button setAttributedTitle:titleSelected forState:UIControlStateHighlighted];
  [button setAttributedTitle:titleSelected forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary *primaryNormal =
    @{NSFontAttributeName: primaryFont, NSForegroundColorAttributeName: [UIColor blackPrimaryText]};
  NSDictionary *secondaryNormal =
    @{NSFontAttributeName: secondaryFont, NSForegroundColorAttributeName: [UIColor blackSecondaryText]};
  NSAttributedString *titleNormal = makeString(primaryText, primaryNormal, secondaryText, secondaryNormal);
  [button setAttributedTitle:titleNormal forState:UIControlStateNormal];

  button.titleLabel.textAlignment = NSTextAlignmentCenter;
}
}  // namespace

@interface MWMSearchHotelsFilterViewController () <UICollectionViewDelegate,
                                                   UICollectionViewDataSource,
                                                   UITableViewDataSource> {
  std::unordered_set<ftypes::IsHotelChecker::Type> m_selectedTypes;
}

@property(nonatomic) MWMFilterRatingCell *rating;
@property(nonatomic) MWMFilterPriceCategoryCell *price;
@property(nonatomic) MWMFilterCollectionHolderCell *type;
@property(nonatomic) MWMHotelParams *filter;
@property(weak, nonatomic) IBOutlet UITableView *tableView;
@property(weak, nonatomic) IBOutlet UIButton *doneButton;

@end

@implementation MWMSearchHotelsFilterViewController

+ (MWMSearchHotelsFilterViewController *)controller {
  NSString *identifier = [self className];
  return static_cast<MWMSearchHotelsFilterViewController *>([self controllerWithIdentifier:identifier]);
}

- (void)applyParams:(MWMHotelParams *)params {
  self.filter = params;
  m_selectedTypes = params.types;
  [self.type.collectionView reloadData];

  using namespace place_page::rating;
  auto ratingCell = self.rating;
  ratingCell.any.selected = NO;
  switch (params.rating) {
    case FilterRating::Any:
      ratingCell.any.selected = YES;
      break;
    case FilterRating::Good:
      ratingCell.good.selected = YES;
      break;
    case FilterRating::VeryGood:
      ratingCell.veryGood.selected = YES;
      break;
    case FilterRating::Excellent:
      ratingCell.excellent.selected = YES;
      break;
  }

  auto priceCell = self.price;
  for (auto const filter : params.price) {
    switch (filter) {
      case Price::Any:
        break;
      case Price::One:
        priceCell.one.selected = YES;
        break;
      case Price::Two:
        priceCell.two.selected = YES;
        break;
      case Price::Three:
        priceCell.three.selected = YES;
        break;
    }
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self configNavigationBar:self.navigationController.navigationBar];
  [self configNavigationItem:self.navigationItem];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  [Statistics logEvent:kStatSearchFilterOpen withParameters:@{kStatCategory: kStatHotel,
                                                              kStatNetwork: [Statistics connectionTypeString]}];
  [self.tableView reloadData];
  [self refreshAppearance];
  [self setNeedsStatusBarAppearanceUpdate];
}

- (void)reset {
  self.rating = nil;
  self.price = nil;
  self.type = nil;
  [self.tableView reloadData];
}

- (void)refreshStatusBarAppearance {
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)refreshViewAppearance {
  self.tableView.backgroundColor = [UIColor clearColor];
  self.tableView.contentInset = {-20, 0, 80, 0};
}

- (void)refreshDoneButtonAppearance {
  UIButton *doneButton = self.doneButton;
  doneButton.backgroundColor = [UIColor linkBlue];
  doneButton.titleLabel.font = [UIFont bold16];
  [doneButton setTitle:L(@"search") forState:UIControlStateNormal];
  [doneButton setTitleColor:[UIColor white] forState:UIControlStateNormal];
}

- (void)refreshAppearance {
  [self refreshStatusBarAppearance];
  [self refreshViewAppearance];
  [self refreshDoneButtonAppearance];
}

- (IBAction)applyAction {
  [MWMEye bookingFilterUsed];
  [self.delegate hotelsFilterViewController:self didSelectParams:[self getSelectedHotelParams]];
}

- (MWMHotelParams *)getSelectedHotelParams {
  MWMHotelParams *params = self.filter != nil ? self.filter : [MWMHotelParams new];
  params.types = m_selectedTypes;

  using namespace place_page::rating;
  MWMFilterRatingCell *rating = self.rating;
  if (rating.good.selected)
    params.rating = FilterRating::Good;
  else if (rating.veryGood.selected)
    params.rating = FilterRating::VeryGood;
  else if (rating.excellent.selected)
    params.rating = FilterRating::Excellent;

  MWMFilterPriceCategoryCell *price = self.price;
  std::unordered_set<Price> priceFilter;
  if (price.one.selected)
    priceFilter.insert(Price::One);
  if (price.two.selected)
    priceFilter.insert(Price::Two);
  if (price.three.selected)
    priceFilter.insert(Price::Three);
  params.price = priceFilter;

  return params;
}

- (void)initialRatingConfig {
  MWMFilterRatingCell *rating = self.rating;
  configButton(rating.any, L(@"booking_filters_rating_any"), nil);
  configButton(rating.good, L(@"7.0+"), L(@"booking_filters_ragting_good"));
  configButton(rating.veryGood, L(@"8.0+"), L(@"booking_filters_rating_very_good"));
  configButton(rating.excellent, L(@"9.0+"), L(@"booking_filters_rating_excellent"));
}

- (void)initialPriceCategoryConfig {
  MWMFilterPriceCategoryCell *price = self.price;
  configButton(price.one, L(@"$"), nil);
  configButton(price.two, L(@"$$"), nil);
  configButton(price.three, L(@"$$$"), nil);
}

- (void)initialTypeConfig {
  [self.type configWithTableView:self.tableView];
}

- (void)resetRating {
  MWMFilterRatingCell *rating = self.rating;
  rating.any.selected = YES;
  rating.good.selected = NO;
  rating.veryGood.selected = NO;
  rating.excellent.selected = NO;
}

- (void)resetPriceCategory {
  MWMFilterPriceCategoryCell *price = self.price;
  price.one.selected = NO;
  price.two.selected = NO;
  price.three.selected = NO;
}

- (void)resetTypes {
  m_selectedTypes.clear();
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
  return base::Underlying(Section::Count);
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = nil;
  switch (static_cast<Section>(indexPath.section)) {
    case Section::Rating:
      cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterRatingCell className] forIndexPath:indexPath];
      if (self.rating != cell) {
        self.rating = static_cast<MWMFilterRatingCell *>(cell);
        [self resetRating];
      }
      [self initialRatingConfig];
      break;
    case Section::PriceCategory:
      cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterPriceCategoryCell className]
                                             forIndexPath:indexPath];
      if (self.price != cell) {
        self.price = static_cast<MWMFilterPriceCategoryCell *>(cell);
        [self resetPriceCategory];
      }
      [self initialPriceCategoryConfig];
      break;
    case Section::Type:
      cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterCollectionHolderCell className]
                                             forIndexPath:indexPath];
      if (self.type != cell) {
        self.type = static_cast<MWMFilterCollectionHolderCell *>(cell);
        [self resetTypes];
      }
      [self initialTypeConfig];
      break;
    case Section::Count:
      NSAssert(false, @"Incorrect section!");
      break;
  }
  return cell;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
  switch (static_cast<Section>(section)) {
    case Section::Rating:
      return L(@"booking_filters_rating");
    case Section::PriceCategory:
      return L(@"booking_filters_price_category");
    case Section::Type:
      return L(@"search_hotel_filters_type");
    default:
      return nil;
  }
}

#pragma mark - UICollectionViewDelegate & UICollectionViewDataSource

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView
                  cellForItemAtIndexPath:(NSIndexPath *)indexPath {
  MWMFilterTypeCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:[MWMFilterTypeCell className]
                                                                      forIndexPath:indexPath];
  auto const type = kTypes[indexPath.row];
  auto str = [NSString stringWithFormat:kHotelTypePattern, @(ftypes::IsHotelChecker::GetHotelTypeTag(type))];
  cell.tagName.text = L(str);

  // we need to do this because of bug - ios 12 doesnt apply layout to cells until scrolling
  if (@available(iOS 12.0, *)) {
    [cell layoutIfNeeded];
  }

  auto const selected = m_selectedTypes.find(type) != m_selectedTypes.end();
  cell.selected = selected;
  if (selected) {
    [collectionView selectItemAtIndexPath:indexPath animated:NO scrollPosition:UICollectionViewScrollPositionNone];
  }

  return cell;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
  return kTypes.size();
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
  auto const type = kTypes[indexPath.row];

  auto typeString = @"";
  switch (type) {
    case ftypes::IsHotelChecker::Type::Hotel:
      typeString = kStatHotel;
      break;
    case ftypes::IsHotelChecker::Type::Apartment:
      typeString = kStatApartment;
      break;
    case ftypes::IsHotelChecker::Type::CampSite:
      typeString = kStatCampSite;
      break;
    case ftypes::IsHotelChecker::Type::Chalet:
      typeString = kStatChalet;
      break;
    case ftypes::IsHotelChecker::Type::GuestHouse:
      typeString = kStatGuestHouse;
      break;
    case ftypes::IsHotelChecker::Type::Hostel:
      typeString = kStatHostel;
      break;
    case ftypes::IsHotelChecker::Type::Motel:
      typeString = kStatMotel;
      break;
    case ftypes::IsHotelChecker::Type::Resort:
      typeString = kStatResort;
      break;
    case ftypes::IsHotelChecker::Type::Count:
      break;
  }
  [Statistics logEvent:kStatSearchFilterClick withParameters:@{kStatCategory: kStatHotel, kStatType: typeString}];
  m_selectedTypes.emplace(type);
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath {
  auto const type = kTypes[indexPath.row];
  m_selectedTypes.erase(type);
}

#pragma mark - Navigation bar

- (IBAction)closeAction {
  [Statistics logEvent:kStatSearchFilterCancel withParameters:@{kStatCategory: kStatHotel}];
  [self.delegate hotelsFilterViewControllerDidCancel:self];
}

- (IBAction)resetAction {
  [Statistics logEvent:kStatSearchFilterReset withParameters:@{kStatCategory: kStatHotel}];
  [self reset];
}

- (void)configNavigationBar:(UINavigationBar *)navBar {
  if (IPAD) {
    UIColor *white = [UIColor white];
    navBar.tintColor = white;
    navBar.barTintColor = white;
    navBar.translucent = NO;
  }
  navBar.titleTextAttributes = @{
    NSForegroundColorAttributeName: IPAD ? [UIColor blackPrimaryText] : [UIColor whiteColor],
    NSFontAttributeName: [UIFont bold17]
  };
}

- (void)configNavigationItem:(UINavigationItem *)navItem {
  UIFont *textFont = [UIFont regular17];

  UIColor *normalStateColor = IPAD ? [UIColor linkBlue] : [UIColor whiteColor];
  UIColor *highlightedStateColor = IPAD ? [UIColor linkBlueHighlighted] : [UIColor whiteColor];
  UIColor *disabledStateColor = [UIColor lightGrayColor];

  navItem.title = L(@"booking_filters");
  navItem.rightBarButtonItem.title = L(@"booking_filters_reset");

  [navItem.rightBarButtonItem
    setTitleTextAttributes:@{NSForegroundColorAttributeName: normalStateColor, NSFontAttributeName: textFont}
                  forState:UIControlStateNormal];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: highlightedStateColor,
  }
                                            forState:UIControlStateHighlighted];
  [navItem.rightBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: disabledStateColor,
  }
                                            forState:UIControlStateDisabled];

  [navItem.leftBarButtonItem
    setTitleTextAttributes:@{NSForegroundColorAttributeName: normalStateColor, NSFontAttributeName: textFont}
                  forState:UIControlStateNormal];
  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: highlightedStateColor,
  }
                                           forState:UIControlStateHighlighted];

  [navItem.leftBarButtonItem setTitleTextAttributes:@{
    NSForegroundColorAttributeName: disabledStateColor,
  }
                                           forState:UIControlStateDisabled];
}

@end
