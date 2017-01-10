#import "MWMSearchHotelsFilterViewController.h"
#import "MWMSearchFilterViewController_Protected.h"

namespace
{
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

@interface MWMSearchHotelsFilterViewController ()

@property(nonatomic) IBOutletCollection(UIButton) NSArray * ratings;

@property(weak, nonatomic) IBOutlet UIButton * ratingAny;
@property(weak, nonatomic) IBOutlet UIButton * rating7;
@property(weak, nonatomic) IBOutlet UIButton * rating8;
@property(weak, nonatomic) IBOutlet UIButton * rating9;

@property(weak, nonatomic) IBOutlet UIButton * price1;
@property(weak, nonatomic) IBOutlet UIButton * price2;
@property(weak, nonatomic) IBOutlet UIButton * price3;

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
  configButton(self.ratingAny, L(@"booking_filters_rating_any"), nil);
  configButton(self.rating7, L(@"7.0+"), L(@"booking_filters_ragting_good"));
  configButton(self.rating8, L(@"8.0+"), L(@"booking_filters_rating_very_good"));
  configButton(self.rating9, L(@"9.0+"), L(@"booking_filters_rating_excellent"));

  configButton(self.price1, L(@"$"), nil);
  configButton(self.price2, L(@"$$"), nil);
  configButton(self.price3, L(@"$$$"), nil);

  [self changeRating:self.ratingAny];

  self.price1.selected = NO;
  self.price2.selected = NO;
  self.price3.selected = NO;
}

- (shared_ptr<search::hotels_filter::Rule>)rules
{
  using namespace search::hotels_filter;
  shared_ptr<Rule> ratingRule;
  if (self.rating7.selected)
    ratingRule = Ge<Rating>(7.0);
  else if (self.rating8.selected)
    ratingRule = Ge<Rating>(8.0);
  else if (self.rating9.selected)
    ratingRule = Ge<Rating>(9.0);

  shared_ptr<Rule> priceRule;
  if (self.price1.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(1));
  if (self.price2.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(2));
  if (self.price3.selected)
    priceRule = Or(priceRule, Eq<PriceRate>(3));

  if (!ratingRule && !priceRule)
    return nullptr;

  return And(ratingRule, priceRule);
}

#pragma mark - Actions

- (IBAction)changeRating:(UIButton *)sender
{
  for (UIButton * button in self.ratings)
    button.selected = NO;
  sender.selected = YES;
}

- (IBAction)priceChange:(UIButton *)sender { sender.selected = !sender.selected; }
#pragma mark - UITableViewDataSource

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (section)
  {
  case 0: return L(@"booking_filters_rating");
  case 1: return L(@"booking_filters_price_category");
  default: return nil;
  }
}

@end
