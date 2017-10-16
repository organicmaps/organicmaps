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
  Thor,
  Spacer
};

NSString * titleForButton(EButton type, BOOL isSelected);

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
                  isSelected:(BOOL)isSelected;

- (EButton)type;
- (MWMCircularProgress *)mapDownloadProgress;


@end
