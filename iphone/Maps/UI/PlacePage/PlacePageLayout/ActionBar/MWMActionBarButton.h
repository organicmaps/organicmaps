enum class EButton  // Required button's order
{
  Api,
  Download,
  Booking,
  BookingSearch,
  Opentable,
  Call,
  Bookmark,
  RouteFrom,
  RouteTo,
  Share,
  More,
  AddStop,
  RemoveStop,
  Partner,
  Spacer
};

NSString * titleForButton(EButton type, int partnerIndex, BOOL isSelected);

@class MWMActionBarButton;
@class MWMCircularProgress;

@protocol MWMActionBarButtonDelegate <NSObject>

- (void)tapOnButtonWithType:(EButton)type;

@end

@interface MWMActionBarButton : UIView

- (void)configButtonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate type:(EButton)type isSelected:(BOOL)isSelected;

+ (void)addButtonToSuperview:(UIView *)view
                    delegate:(id<MWMActionBarButtonDelegate>)delegate
                  buttonType:(EButton)type
                partnerIndex:(int)partnerIndex
                  isSelected:(BOOL)isSelected;

- (EButton)type;
- (MWMCircularProgress *)mapDownloadProgress;
- (int)partnerIndex;

@end
