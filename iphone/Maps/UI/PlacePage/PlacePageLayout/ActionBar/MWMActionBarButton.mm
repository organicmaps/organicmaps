#import "MWMActionBarButton.h"
#import "MWMCommon.h"
#import "MWMButton.h"
#import "MWMCircularProgress.h"

NSString * titleForButton(EButton type, BOOL isSelected)
{
  switch (type)
  {
  case EButton::Api:
    return L(@"back");
  case EButton::Download:
    return L(@"download");
  case EButton::Booking:
  case EButton::Opentable:
    return L(@"book_button");
  case EButton::BookingSearch:
    return L(@"booking_search");
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
  case EButton::AddStop:
    return L(@"placepage_add_stop");
  case EButton::RemoveStop:
    return L(@"placepage_remove_stop");
  case EButton::Thor:
    return L(@"sponsored_thor_gather");
  case EButton::Spacer:
    return nil;
  }
}

@interface MWMActionBarButton () <MWMCircularProgressProtocol>

@property (weak, nonatomic) IBOutlet MWMButton * button;
@property (weak, nonatomic) IBOutlet UILabel * label;

@property (weak, nonatomic) id<MWMActionBarButtonDelegate> delegate;
@property (nonatomic) EButton type;
@property(nonatomic) MWMCircularProgress * mapDownloadProgress;
@property(nonatomic) UIView * progressWrapper;

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
  case EButton::Download:
  {
    if (self.mapDownloadProgress)
      return;

    self.progressWrapper = [[UIView alloc] init];
    [self.button addSubview:self.progressWrapper];

    self.mapDownloadProgress = [MWMCircularProgress downloaderProgressForParentView:self.progressWrapper];
      self.mapDownloadProgress.delegate = self;

    MWMCircularProgressStateVec const affectedStates = {MWMCircularProgressStateNormal,
      MWMCircularProgressStateSelected};

    [self.mapDownloadProgress setImageName:@"ic_download" forStates:affectedStates];
    [self.mapDownloadProgress setColoring:MWMButtonColoringBlue forStates:affectedStates];
    break;
  }
  case EButton::Booking:
    [self.button setImage:[UIImage imageNamed:@"ic_booking_logo"] forState:UIControlStateNormal];
    self.label.textColor = UIColor.whiteColor;
    self.backgroundColor = [UIColor bookingBackground];
    break;
  case EButton::BookingSearch:
    [self.button setImage:[UIImage imageNamed:@"ic_booking_search"] forState:UIControlStateNormal];
    self.label.textColor = UIColor.whiteColor;
    self.backgroundColor = [UIColor bookingBackground];
    break;
  case EButton::Opentable:
    [self.button setImage:[UIImage imageNamed:@"ic_opentable"] forState:UIControlStateNormal];
    self.label.textColor = UIColor.whiteColor;
    self.backgroundColor = [UIColor opentableBackground];
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
  case EButton::AddStop:
    [self.button setImage:[UIImage imageNamed:@"ic_add_route_point"] forState:UIControlStateNormal];
    break;
  case EButton::RemoveStop:
    [self.button setImage:[UIImage imageNamed:@"ic_remove_route_point"] forState:UIControlStateNormal];
    break;
  case EButton::Spacer:
    [self.button removeFromSuperview];
    [self.label removeFromSuperview];
    break;
  case EButton::Thor:
    [self.button setImage:[UIImage imageNamed:@"ic_28px_logo_thor"] forState:UIControlStateNormal];
    self.label.textColor = UIColor.whiteColor;
    self.backgroundColor = UIColor.thorBackground;
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
  MWMActionBarButton * button =
      [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  button.delegate = delegate;
  button.type = type;
  [view addSubview:button];
  [button configButton:isSelected];
}

- (void)progressButtonPressed:(MWMCircularProgress *)progress
{
  [self.delegate tapOnButtonWithType:EButton::Download];
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
  {
    UIImage * image = [UIImage imageNamed:[NSString stringWithFormat:@"ic_bookmarks_%@", @(i + 1)]];
    animationImages[i] = image;
  }
  UIImageView * animationIV = btn.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationRepeatCount = 1;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.frame = self.superview.bounds;
  CGFloat constexpr designOffset = 4;
  self.progressWrapper.size = {self.button.height - designOffset, self.button.height - designOffset};
  self.progressWrapper.center = self.button.center;
}

@end
