enum class EButton  // Required button's order
{
  Booking,
  BookingSearch,
  Bookmark,
  Call,
  Download,
  More,
  Opentable,
  Partner,
  RouteAddStop,
  RouteFrom,
  RouteRemoveStop,
  RouteTo,
  Share
};

NSString * titleForButton(EButton type, int partnerIndex, BOOL isSelected);

@class MWMActionBarButton;
@class MWMCircularProgress;

@protocol MWMActionBarButtonDelegate <NSObject>

- (void)tapOnButtonWithType:(EButton)type;

@end

@interface MWMActionBarButton : UIView

+ (MWMActionBarButton *)buttonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate
                                buttonType:(EButton)type
                              partnerIndex:(int)partnerIndex
                                isSelected:(BOOL)isSelected;

- (EButton)type;
- (MWMCircularProgress *)mapDownloadProgress;
- (int)partnerIndex;

@end
