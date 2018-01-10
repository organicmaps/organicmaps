#import "MWMSearchHotelsFilterViewController.h"
#import <CoreActionSheetPicker/ActionSheetPicker.h>
#import "MWMSearch.h"
#import "MWMSearchFilterViewController_Protected.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "search/hotels_filter.hpp"

namespace
{
static NSTimeInterval kDayInterval = 24 * 60 * 60;
static NSTimeInterval k30DaysInterval = 30 * kDayInterval;
static NSTimeInterval k360DaysInterval = 360 * kDayInterval;
static uint8_t kAdultsCount = 2;
static int8_t kAgeOfChild = 5;
static NSString * const kHotelTypePattern = @"search_hotel_filter_%@";

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
  Check,
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
  UIFont * primaryFont = [UIFont regular14];
  UIFont * secondaryFont = [UIFont medium10];

  UIColor * white = [UIColor white];

  UIImage * linkBlueImage = [UIImage imageWithColor:[UIColor linkBlue]];
  UIImage * linkBlueHighlightedImage = [UIImage imageWithColor:[UIColor linkBlueHighlighted]];

  [button setBackgroundImage:[UIImage imageWithColor:white] forState:UIControlStateNormal];
  [button setBackgroundImage:linkBlueImage forState:UIControlStateSelected];
  [button setBackgroundImage:linkBlueHighlightedImage forState:UIControlStateHighlighted];
  [button setBackgroundImage:linkBlueHighlightedImage
                    forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary * primarySelected =
      @{NSFontAttributeName: primaryFont, NSForegroundColorAttributeName: white};
  NSDictionary * secondarySelected =
      @{NSFontAttributeName: secondaryFont, NSForegroundColorAttributeName: white};
  NSAttributedString * titleSelected =
      makeString(primaryText, primarySelected, secondaryText, secondarySelected);
  [button setAttributedTitle:titleSelected forState:UIControlStateSelected];
  [button setAttributedTitle:titleSelected forState:UIControlStateHighlighted];
  [button setAttributedTitle:titleSelected
                    forState:UIControlStateSelected | UIControlStateHighlighted];

  NSDictionary * primaryNormal = @{
    NSFontAttributeName: primaryFont,
    NSForegroundColorAttributeName: [UIColor blackPrimaryText]
  };
  NSDictionary * secondaryNormal = @{
    NSFontAttributeName: secondaryFont,
    NSForegroundColorAttributeName: [UIColor blackSecondaryText]
  };
  NSAttributedString * titleNormal =
      makeString(primaryText, primaryNormal, secondaryText, secondaryNormal);
  [button setAttributedTitle:titleNormal forState:UIControlStateNormal];

  button.titleLabel.textAlignment = NSTextAlignmentCenter;
}
}  // namespace

@interface MWMSearchHotelsFilterViewController ()<
    UICollectionViewDelegate, UICollectionViewDataSource, MWMFilterCheckCellDelegate>
{
  std::vector<ftypes::IsHotelChecker::Type> m_selectedTypes;
}

@property(nonatomic) MWMFilterCheckCell * check;
@property(nonatomic) MWMFilterRatingCell * rating;
@property(nonatomic) MWMFilterPriceCategoryCell * price;
@property(nonatomic) MWMFilterCollectionHolderCell * type;
@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet UIButton * doneButton;

@property(nonatomic) NSDate * checkInDate;
@property(nonatomic) NSDate * checkOutDate;

@end

@implementation MWMSearchHotelsFilterViewController

+ (MWMSearchHotelsFilterViewController *)controller
{
  NSString * identifier = [self className];
  return static_cast<MWMSearchHotelsFilterViewController *>(
      [self controllerWithIdentifier:identifier]);
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [Statistics logEvent:kStatSearchFilterOpen withParameters:@{kStatCategory: kStatHotel}];
  [self.tableView reloadData];
  [self refreshAppearance];
  [self setNeedsStatusBarAppearanceUpdate];
}

- (void)reset
{
  self.check = nil;
  self.rating = nil;
  self.price = nil;
  self.type = nil;
  [self.tableView reloadData];
}

- (void)refreshStatusBarAppearance
{
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)refreshViewAppearance
{
  self.view.backgroundColor = [UIColor pressBackground];
  self.tableView.backgroundColor = [UIColor clearColor];
}

- (void)refreshDoneButtonAppearance
{
  UIButton * doneButton = self.doneButton;
  doneButton.backgroundColor = [UIColor linkBlue];
  doneButton.titleLabel.font = [UIFont bold16];
  [doneButton setTitle:L(@"done") forState:UIControlStateNormal];
  [doneButton setTitleColor:[UIColor white] forState:UIControlStateNormal];
}

- (void)refreshAppearance
{
  [self refreshStatusBarAppearance];
  [self refreshViewAppearance];
  [self refreshDoneButtonAppearance];
}

- (IBAction)applyAction
{
  [Statistics logEvent:kStatSearchFilterApply withParameters:@{kStatCategory: kStatHotel}];
  [MWMSearch update];
  [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)initialCheckConfig
{
  MWMFilterCheckCell * check = self.check;
  [check refreshLabelsAppearance];
  [check refreshButtonsAppearance];
  check.isOffline = !Platform::IsConnected();
  check.delegate = self;
}

- (void)initialRatingConfig
{
  MWMFilterRatingCell * rating = self.rating;
  configButton(rating.any, L(@"booking_filters_rating_any"), nil);
  configButton(rating.good, L(@"7.0+"), L(@"booking_filters_ragting_good"));
  configButton(rating.veryGood, L(@"8.0+"), L(@"booking_filters_rating_very_good"));
  configButton(rating.excellent, L(@"9.0+"), L(@"booking_filters_rating_excellent"));
}

- (void)initialPriceCategoryConfig
{
  MWMFilterPriceCategoryCell * price = self.price;
  configButton(price.one, L(@"$"), nil);
  configButton(price.two, L(@"$$"), nil);
  configButton(price.three, L(@"$$$"), nil);
}

- (void)initialTypeConfig
{
  [self.type configWithTableView:self.tableView];
}

- (void)resetCheck
{
  self.checkInDate = [NSDate date];
  self.checkOutDate = [[NSDate date] dateByAddingTimeInterval:kDayInterval];
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

- (booking::filter::availability::Params)availabilityParams
{
  using Clock = booking::AvailabilityParams::Clock;
  booking::filter::availability::Params params;
  params.m_params.m_rooms = {{kAdultsCount, kAgeOfChild}};
  params.m_params.m_checkin = Clock::from_time_t(self.checkInDate.timeIntervalSince1970);
  params.m_params.m_checkout = Clock::from_time_t(self.checkOutDate.timeIntervalSince1970);
  return params;
}

#pragma mark - MWMFilterCheckCellDelegate

- (void)checkCellButtonTap:(UIButton * _Nonnull)button
{
  NSString * title;
  NSDate * selectedDate;
  NSDate * minimumDate;
  NSDate * maximumDate;
  if (button == self.check.checkIn)
  {
    [Statistics logEvent:kStatSearchFilterClick
          withParameters:@{kStatCategory: kStatHotel, kStatDate: kStatCheckIn}];
    title = L(@"booking_filters_check_in");
    selectedDate = self.checkInDate;
    minimumDate = [[NSDate date] earlierDate:self.checkOutDate];
    maximumDate = [minimumDate dateByAddingTimeInterval:k360DaysInterval];
  }
  else
  {
    [Statistics logEvent:kStatSearchFilterClick
          withParameters:@{kStatCategory: kStatHotel, kStatDate: kStatCheckOut}];
    title = L(@"booking_filters_check_out");
    selectedDate = self.checkOutDate;
    minimumDate =
        [[[NSDate date] laterDate:self.checkInDate] dateByAddingTimeInterval:kDayInterval];
    maximumDate = [self.checkInDate dateByAddingTimeInterval:k30DaysInterval];
  }

  auto picker = [[ActionSheetDatePicker alloc] initWithTitle:title
                                              datePickerMode:UIDatePickerModeDate
                                                selectedDate:selectedDate
                                                 minimumDate:minimumDate
                                                 maximumDate:maximumDate
                                                      target:self
                                                      action:@selector(datePickerDate:origin:)
                                                      origin:button];
  picker.tapDismissAction = TapActionCancel;
  picker.hideCancel = YES;
  auto doneButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                                  target:nil
                                                                  action:nil];
  [doneButton setTitleTextAttributes:@{NSForegroundColorAttributeName: [UIColor linkBlue]}
                            forState:UIControlStateNormal];
  [picker setDoneButton:doneButton];
  [picker showActionSheetPicker];
}

- (void)datePickerDate:(NSDate *)date origin:(id)origin
{
  if (origin == self.check.checkIn)
    self.checkInDate = date;
  else
    self.checkOutDate = date;
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
  case Section::Check:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterCheckCell className]
                                           forIndexPath:indexPath];
    if (!self.check)
    {
      self.check = static_cast<MWMFilterCheckCell *>(cell);
      [self resetCheck];
    }
    [self initialCheckConfig];
    break;
  case Section::Rating:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterRatingCell className] forIndexPath:indexPath];
    if (!self.rating)
    {
      self.rating = static_cast<MWMFilterRatingCell *>(cell);
      [self resetRating];
    }
    [self initialRatingConfig];
    break;
  case Section::PriceCategory:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterPriceCategoryCell className] forIndexPath:indexPath];
    if (!self.price)
    {
      self.price = static_cast<MWMFilterPriceCategoryCell *>(cell);
      [self resetPriceCategory];
    }
    [self initialPriceCategoryConfig];
    break;
  case Section::Type:
    cell = [tableView dequeueReusableCellWithIdentifier:[MWMFilterCollectionHolderCell className] forIndexPath:indexPath];
    if (!self.type)
    {
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

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (static_cast<Section>(section))
  {
  case Section::Rating: return L(@"booking_filters_rating");
  case Section::PriceCategory: return L(@"booking_filters_price_category");
  case Section::Type: return L(@"search_hotel_filters_type");
  default: return nil;
  }
}

#pragma mark - UICollectionViewDelegate & UICollectionViewDataSource

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  MWMFilterTypeCell * cell = [collectionView dequeueReusableCellWithReuseIdentifier:[MWMFilterTypeCell className]
                                                                       forIndexPath:indexPath];
  auto const type = kTypes[indexPath.row];
  auto str = [NSString stringWithFormat:kHotelTypePattern, @(ftypes::IsHotelChecker::GetHotelTypeTag(type))];
  cell.tagName.text = L(str);
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

  auto typeString = @"";
  switch (type)
  {
  case ftypes::IsHotelChecker::Type::Hotel: typeString = kStatHotel; break;
  case ftypes::IsHotelChecker::Type::Apartment: typeString = kStatApartment; break;
  case ftypes::IsHotelChecker::Type::CampSite: typeString = kStatCampSite; break;
  case ftypes::IsHotelChecker::Type::Chalet: typeString = kStatChalet; break;
  case ftypes::IsHotelChecker::Type::GuestHouse: typeString = kStatGuestHouse; break;
  case ftypes::IsHotelChecker::Type::Hostel: typeString = kStatHostel; break;
  case ftypes::IsHotelChecker::Type::Motel: typeString = kStatMotel; break;
  case ftypes::IsHotelChecker::Type::Resort: typeString = kStatResort; break;
  case ftypes::IsHotelChecker::Type::Count: break;
  }
  [Statistics logEvent:kStatSearchFilterClick
        withParameters:@{kStatCategory: kStatHotel, kStatType: typeString}];
  m_selectedTypes.emplace_back(type);
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
  auto const type = kTypes[indexPath.row];
  m_selectedTypes.erase(remove(m_selectedTypes.begin(), m_selectedTypes.end(), type));
}

#pragma mark - Properties

- (void)setCheckInDate:(NSDate *)checkInDate
{
  _checkInDate = checkInDate;
  if (auto check = self.check)
  {
    [check.checkIn setTitle:[NSDateFormatter localizedStringFromDate:checkInDate
                                                           dateStyle:NSDateFormatterMediumStyle
                                                           timeStyle:NSDateFormatterNoStyle]
                   forState:UIControlStateNormal];
  }
  self.checkOutDate = [[checkInDate dateByAddingTimeInterval:k30DaysInterval]
      earlierDate:[[checkInDate dateByAddingTimeInterval:kDayInterval]
                      laterDate:self.checkOutDate]];
}

- (void)setCheckOutDate:(NSDate *)checkOutDate
{
  _checkOutDate = checkOutDate;
  if (auto check = self.check)
  {
    [check.checkOut setTitle:[NSDateFormatter localizedStringFromDate:checkOutDate
                                                            dateStyle:NSDateFormatterMediumStyle
                                                            timeStyle:NSDateFormatterNoStyle]
                    forState:UIControlStateNormal];
  }
}

@end
