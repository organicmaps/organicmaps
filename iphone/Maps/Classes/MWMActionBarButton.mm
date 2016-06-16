#import "MWMActionBarButton.h"
#import "MWMButton.h"
#import "UIColor+MapsMeColor.h"

NSString * titleForButton(EButton type, BOOL isSelected)
{
  switch (type)
  {
  case EButton::Api:
    return L(@"back");
  case EButton::Booking:
    return L(@"bookingcom_book_button");
  case EButton::Call:
    return L(@"placepage_call_button");
  case EButton::Bookmark:
    return L(isSelected ? @"delete" : @"save");
  case EButton::RouteFrom:
    return L(@"p2p_from_here");
  case EButton::RouteTo:
    return L(@"p2p_to_here");
  case EButton::Share:
    return L(@"share");
  case EButton::More:
    return L(@"placepage_more_button");
  case EButton::Spacer:
    return nil;
  }
}

@interface MWMActionBarButton ()

@property (weak, nonatomic) IBOutlet MWMButton * button;
@property (weak, nonatomic) IBOutlet UILabel * label;

@property (weak, nonatomic) id<MWMActionBarButtonDelegate> delegate;
@property (nonatomic) EButton type;

@end

@implementation MWMActionBarButton

- (void)configButtonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate type:(EButton)type isSelected:(BOOL)isSelected
{
  self.delegate = delegate;
  self.type = type;
  [self configButton:isSelected];
}

- (void)configButton:(BOOL)isSelected
{
  self.label.text = titleForButton(self.type, isSelected);
  switch (self.type)
  {
  case EButton::Api:
    [self.button setImage:[UIImage imageNamed:@"ic_back_api"] forState:UIControlStateNormal];
    break;
  case EButton::Booking:
    [self.button setImage:[UIImage imageNamed:@"ic_booking_logo"] forState:UIControlStateNormal];
    self.label.textColor = [UIColor whiteColor];
    self.backgroundColor = [UIColor bookingBackground];
    break;
  case EButton::Call:
    [self.button setImage:[UIImage imageNamed:@"ic_placepage_phone_number"] forState:UIControlStateNormal];
    break;
  case EButton::Bookmark:
    [self setupBookmarkButton:isSelected];
    break;
  case EButton::RouteFrom:
    [self.button setImage:[UIImage imageNamed:@"ic_route_from"] forState:UIControlStateNormal];
    break;
  case EButton::RouteTo:
    [self.button setImage:[UIImage imageNamed:@"ic_route_to"] forState:UIControlStateNormal];
    break;
  case EButton::Share:
    [self.button setImage:[UIImage imageNamed:@"ic_menu_share"] forState:UIControlStateNormal];
    break;
  case EButton::More:
    [self.button setImage:[UIImage imageNamed:@"ic_placepage_more"] forState:UIControlStateNormal];
    break;
  case EButton::Spacer:
    [self.button removeFromSuperview];
    [self.label removeFromSuperview];
    break;
  }
}

+ (void)addButtonToSuperview:(UIView *)view
                    delegate:(id<MWMActionBarButtonDelegate>)delegate
                  buttonType:(EButton)type
                  isSelected:(BOOL)isSelected
{
  if (view.subviews.count)
    return;
  MWMActionBarButton * button = [[[NSBundle mainBundle] loadNibNamed:[MWMActionBarButton className] owner:nil options:nil] firstObject];
  button.delegate = delegate;
  button.type = type;
  [view addSubview:button];
  [button configButton:isSelected];
}

- (IBAction)tap
{
  if (self.type == EButton::Bookmark)
    [self setBookmarkSelected:!self.button.isSelected];

  [self.delegate tapOnButtonWithType:self.type];
}

- (void)setBookmarkSelected:(BOOL)isSelected
{
  if (isSelected)
    [self.button.imageView startAnimating];

  self.button.selected = isSelected;
  self.label.text = L(isSelected ? @"delete" : @"save");
}

- (void)setupBookmarkButton:(BOOL)isSelected
{
  MWMButton * btn = self.button;
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_off"] forState:UIControlStateNormal];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateSelected];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateHighlighted];

  [self setBookmarkSelected:isSelected];

  NSUInteger const animationImagesCount = 11;
  NSMutableArray * animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i)
    animationImages[i] = [UIImage imageNamed:[NSString stringWithFormat:@"ic_bookmarks_%@", @(i+1)]];

  UIImageView * animationIV = btn.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationRepeatCount = 1;
}

- (void)layoutSubviews
{
  self.frame = self.superview.bounds;
  [super layoutSubviews];
}

@end
