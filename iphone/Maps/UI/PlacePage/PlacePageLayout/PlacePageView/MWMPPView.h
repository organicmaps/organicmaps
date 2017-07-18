#pragma mark - MWMPPScrollView

@protocol MWMPlacePageViewUpdateProtocol<NSObject>

- (void)updateLayout;

@end

@interface MWMPPScrollView : UIScrollView

- (instancetype)initWithFrame:(CGRect)frame inactiveView:(UIView *)inactiveView;

@property(weak, nonatomic) UIView * inactiveView;

@end

#pragma mark - MWMPPView

@interface MWMPPView : UIView

@property(weak, nonatomic) IBOutlet UIView * top;
@property(weak, nonatomic) IBOutlet UIImageView * anchorImage;
@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(nonatomic) CGFloat currentContentHeight;
@property(weak, nonatomic) id<MWMPlacePageViewUpdateProtocol> delegate;

@end
