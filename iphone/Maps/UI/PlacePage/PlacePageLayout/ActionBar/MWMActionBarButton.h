typedef NS_ENUM(NSInteger, MWMActionBarButtonType) {
  MWMActionBarButtonTypeBooking,
  MWMActionBarButtonTypeBookingSearch,
  MWMActionBarButtonTypeBookmark,
  MWMActionBarButtonTypeCall,
  MWMActionBarButtonTypeDownload,
  MWMActionBarButtonTypeMore,
  MWMActionBarButtonTypeOpentable,
  MWMActionBarButtonTypePartner,
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
NSString * titleForButton(MWMActionBarButtonType type, NSInteger partnerIndex, BOOL isSelected);
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
@property(nonatomic, readonly) NSInteger partnerIndex;

+ (MWMActionBarButton *)buttonWithDelegate:(id<MWMActionBarButtonDelegate>)delegate
                                buttonType:(MWMActionBarButtonType)type
                              partnerIndex:(NSInteger)partnerIndex
                                isSelected:(BOOL)isSelected
                                isEnabled:(BOOL)isEnabled;

@end

NS_ASSUME_NONNULL_END
