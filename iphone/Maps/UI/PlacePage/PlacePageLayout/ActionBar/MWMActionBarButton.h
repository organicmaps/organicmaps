typedef NS_ENUM(NSInteger, MWMActionBarButtonType) {
  MWMActionBarButtonTypeBooking,
  MWMActionBarButtonTypeBookingSearch,
  MWMActionBarButtonTypeBookmark,
  MWMActionBarButtonTypeCall,
  MWMActionBarButtonTypeDownload,
  MWMActionBarButtonTypeMore,
  MWMActionBarButtonTypeOpentable,
  MWMActionBarButtonTypeRouteAddStop,
  MWMActionBarButtonTypeRouteFrom,
  MWMActionBarButtonTypeRouteRemoveStop,
  MWMActionBarButtonTypeRouteTo,
  MWMActionBarButtonTypeShare,
  MWMActionBarButtonTypeAvoidToll,
  MWMActionBarButtonTypeAvoidDirty,
  MWMActionBarButtonTypeAvoidFerry
} NS_SWIFT_NAME(ActionBarButtonType);

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif
NSString * titleForButton(MWMActionBarButtonType type, BOOL isSelected);
#ifdef __cplusplus
}
#endif

@class MWMActionBarButton;
@class MWMCircularProgress;

NS_SWIFT_NAME(ActionBarButtonDelegate)
@protocol MWMActionBarButtonDelegate <NSObject>

- (void)tapOnButtonWithType:(MWMActionBarButtonType)type;

@end

NS_SWIFT_NAME(ActionBarButton)
@interface MWMActionBarButton : UIView

@property(nonatomic, readonly) MWMActionBarButtonType type;
@property(nonatomic, readonly, nullable) MWMCircularProgress *mapDownloadProgress;

+ (MWMActionBarButton *)buttonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate
                                buttonType:(MWMActionBarButtonType)type
                                isSelected:(BOOL)isSelected
                                isEnabled:(BOOL)isEnabled;

@end

NS_ASSUME_NONNULL_END
