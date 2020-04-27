#import "MWMActionBarButton.h"
#import "MWMButton.h"
#import "MWMCircularProgress.h"
#import "SwiftBridge.h"

static NSString *titleForPartner(NSInteger partnerIndex) {
  NSString *str = [NSString stringWithFormat:@"sponsored_partner%ld_action", (long)partnerIndex];
  NSString *localizedStr = L(str);
  NSCAssert(![str isEqualToString:localizedStr], @"Localization is absent.");
  return localizedStr;
}

NSString *titleForButton(MWMActionBarButtonType type, NSInteger partnerIndex, BOOL isSelected) {
  switch (type) {
    case MWMActionBarButtonTypeDownload:
      return L(@"download");
    case MWMActionBarButtonTypeBooking:
    case MWMActionBarButtonTypeOpentable:
      return L(@"book_button");
    case MWMActionBarButtonTypeBookingSearch:
      return L(@"booking_search");
    case MWMActionBarButtonTypeCall:
      return L(@"placepage_call_button");
    case MWMActionBarButtonTypeBookmark:
      return L(isSelected ? @"delete" : @"save");
    case MWMActionBarButtonTypeRouteFrom:
      return L(@"p2p_from_here");
    case MWMActionBarButtonTypeRouteTo:
      return L(@"p2p_to_here");
    case MWMActionBarButtonTypeShare:
      return L(@"share");
    case MWMActionBarButtonTypeMore:
      return L(@"placepage_more_button");
    case MWMActionBarButtonTypeRouteAddStop:
      return L(@"placepage_add_stop");
    case MWMActionBarButtonTypeRouteRemoveStop:
      return L(@"placepage_remove_stop");
    case MWMActionBarButtonTypeAvoidToll:
      return L(@"avoid_toll_roads_placepage");
    case MWMActionBarButtonTypeAvoidDirty:
      return L(@"avoid_unpaved_roads_placepage");
    case MWMActionBarButtonTypeAvoidFerry:
      return L(@"avoid_ferry_crossing_placepage");
    case MWMActionBarButtonTypePartner:
      return titleForPartner(partnerIndex);
  }
}

static NSString *imageNameForPartner(NSInteger partnerIndex) {
  return [NSString stringWithFormat:@"ic_28px_logo_partner%ld", (long)partnerIndex];
}

static UIImage *imageForPartner(NSInteger partnerIndex) {
  UIImage *img = [UIImage imageNamed:imageNameForPartner(partnerIndex)];
  NSCAssert(img != nil, @"Partner image is absent.");
  return img;
}

static UIColor *textColorForPartner(NSInteger partnerIndex) {
  NSString *textColor = [NSString stringWithFormat:@"partner%ldTextColor", (long)partnerIndex];
  UIColor *color = [UIColor colorWithName:textColor];
  NSCAssert(color != nil, @"Partner text color is absent.");
  return color;
}

static UIColor *backgroundColorForPartner(NSInteger partnerIndex) {
  NSString *colorName = [NSString stringWithFormat:@"partner%ldBackground", (long)partnerIndex];
  UIColor *color = [UIColor colorWithName:colorName];
  NSCAssert(color != nil, @"Partner background color is absent.");
  return color;
}

@interface MWMActionBarButton () <MWMCircularProgressProtocol>

@property(nonatomic) MWMActionBarButtonType type;
@property(nonatomic) MWMCircularProgress *mapDownloadProgress;
@property(nonatomic) NSInteger partnerIndex;
@property(weak, nonatomic) IBOutlet MWMButton *button;
@property(weak, nonatomic) IBOutlet UILabel *label;
@property(weak, nonatomic) IBOutlet UIView *extraBackground;
@property(weak, nonatomic) id<MWMActionBarButtonDelegate> delegate;

@end

@implementation MWMActionBarButton

- (void)configButton:(BOOL)isSelected enabled:(BOOL)isEnabled {
  self.label.text = titleForButton(self.type, self.partnerIndex, isSelected);
  self.extraBackground.hidden = YES;
  switch (self.type) {
    case MWMActionBarButtonTypeDownload: {
      if (self.mapDownloadProgress)
        return;

      self.mapDownloadProgress = [MWMCircularProgress downloaderProgressForParentView:self.button];
      self.mapDownloadProgress.delegate = self;

      MWMCircularProgressStateVec affectedStates =
        @[@(MWMCircularProgressStateNormal), @(MWMCircularProgressStateSelected)];

      [self.mapDownloadProgress setImageName:@"ic_download" forStates:affectedStates];
      [self.mapDownloadProgress setColoring:MWMButtonColoringBlue forStates:affectedStates];
      break;
    }
    case MWMActionBarButtonTypeBooking:
      [self.button setImage:[UIImage imageNamed:@"ic_booking_logo"] forState:UIControlStateNormal];
      self.label.styleName = @"PPActionBarTitlePartner";
      self.backgroundColor = [UIColor bookingBackground];
      if (!IPAD) {
        self.extraBackground.backgroundColor = [UIColor bookingBackground];
        self.extraBackground.hidden = NO;
      }
      break;
    case MWMActionBarButtonTypeBookingSearch:
      [self.button setImage:[UIImage imageNamed:@"ic_booking_search"] forState:UIControlStateNormal];
      self.label.styleName = @"PPActionBarTitlePartner";
      self.backgroundColor = [UIColor bookingBackground];
      if (!IPAD) {
        self.extraBackground.backgroundColor = [UIColor bookingBackground];
        self.extraBackground.hidden = NO;
      }
      break;
    case MWMActionBarButtonTypeOpentable:
      [self.button setImage:[UIImage imageNamed:@"ic_opentable"] forState:UIControlStateNormal];
      self.label.styleName = @"PPActionBarTitlePartner";
      self.backgroundColor = [UIColor opentableBackground];
      if (!IPAD) {
        self.extraBackground.backgroundColor = [UIColor opentableBackground];
        self.extraBackground.hidden = NO;
      }
      break;
    case MWMActionBarButtonTypeCall:
      [self.button setImage:[UIImage imageNamed:@"ic_placepage_phone_number"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeBookmark:
      [self setupBookmarkButton:isSelected];
      break;
    case MWMActionBarButtonTypeRouteFrom:
      [self.button setImage:[UIImage imageNamed:@"ic_route_from"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeRouteTo:
      [self.button setImage:[UIImage imageNamed:@"ic_route_to"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeShare:
      [self.button setImage:[UIImage imageNamed:@"ic_menu_share"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeMore:
      [self.button setImage:[UIImage imageNamed:@"ic_placepage_more"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeRouteAddStop:
      [self.button setImage:[UIImage imageNamed:@"ic_add_route_point"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeRouteRemoveStop:
      [self.button setImage:[UIImage imageNamed:@"ic_remove_route_point"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypePartner:
      [self.button setImage:imageForPartner(self.partnerIndex) forState:UIControlStateNormal];
      self.label.styleName = @"regular10";
      self.label.textColor = textColorForPartner(self.partnerIndex);
      self.backgroundColor = backgroundColorForPartner(self.partnerIndex);
      break;
    case MWMActionBarButtonTypeAvoidToll:
      [self.button setImage:[UIImage imageNamed:@"ic_avoid_tolls"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeAvoidDirty:
      [self.button setImage:[UIImage imageNamed:@"ic_avoid_dirty"] forState:UIControlStateNormal];
      break;
    case MWMActionBarButtonTypeAvoidFerry:
      [self.button setImage:[UIImage imageNamed:@"ic_avoid_ferry"] forState:UIControlStateNormal];
      break;
  }
  self.button.enabled = isEnabled;
}

+ (MWMActionBarButton *)buttonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate
                                buttonType:(MWMActionBarButtonType)type
                              partnerIndex:(NSInteger)partnerIndex
                                isSelected:(BOOL)isSelected
                                isEnabled:(BOOL)isEnabled {
  MWMActionBarButton *button = [NSBundle.mainBundle loadNibNamed:[self className] owner:nil options:nil].firstObject;
  button.delegate = delegate;
  button.type = type;
  button.partnerIndex = partnerIndex;
  [button configButton:isSelected enabled:isEnabled];
  return button;
}

- (void)progressButtonPressed:(MWMCircularProgress *)progress {
  [self.delegate tapOnButtonWithType:self.type];
}

- (IBAction)tap {
  if (self.type == MWMActionBarButtonTypeBookmark)
    [self setBookmarkSelected:!self.button.isSelected];

  [self.delegate tapOnButtonWithType:self.type];
}

- (void)setBookmarkSelected:(BOOL)isSelected {
  if (isSelected)
    [self.button.imageView startAnimating];

  self.button.selected = isSelected;
  self.label.text = L(isSelected ? @"delete" : @"save");
}

- (void)setupBookmarkButton:(BOOL)isSelected {
  MWMButton *btn = self.button;
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_off"] forState:UIControlStateNormal];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateSelected];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateHighlighted];
  [btn setImage:[UIImage imageNamed:@"ic_bookmarks_on"] forState:UIControlStateDisabled];

  [self setBookmarkSelected:isSelected];

  NSUInteger const animationImagesCount = 11;
  NSMutableArray *animationImages = [NSMutableArray arrayWithCapacity:animationImagesCount];
  for (NSUInteger i = 0; i < animationImagesCount; ++i) {
    UIImage *image = [UIImage imageNamed:[NSString stringWithFormat:@"ic_bookmarks_%@", @(i + 1)]];
    animationImages[i] = image;
  }
  UIImageView *animationIV = btn.imageView;
  animationIV.animationImages = animationImages;
  animationIV.animationRepeatCount = 1;
}

@end
