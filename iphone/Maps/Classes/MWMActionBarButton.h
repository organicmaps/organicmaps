enum class EButton  // Required button's order
{
  Api,
  Booking,
  Opentable,
  Call,
  Bookmark,
  RouteFrom,
  RouteTo,
  Share,
  More,
  Spacer
};

NSString * titleForButton(EButton type, BOOL isSelected);

@class MWMActionBarButton;

@protocol MWMActionBarButtonDelegate <NSObject>

- (void)tapOnButtonWithType:(EButton)type;

@end

@interface MWMActionBarButton : UIView

- (void)configButtonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate type:(EButton)type isSelected:(BOOL)isSelected;

+ (void)addButtonToSuperview:(UIView *)view
                    delegate:(id<MWMActionBarButtonDelegate>)delegate
                  buttonType:(EButton)type
                  isSelected:(BOOL)isSelected;

@end
