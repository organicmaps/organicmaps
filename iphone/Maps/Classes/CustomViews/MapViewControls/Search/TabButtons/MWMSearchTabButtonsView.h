@class MWMSearchTabButtonsView;

@protocol MWMSearchTabButtonsViewProtocol <NSObject>

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender;

@end

IB_DESIGNABLE
@interface MWMSearchTabButtonsView : UIView

@property (nonatomic) BOOL selected;

@property (nonatomic) IBInspectable UIImage * iconImage;
@property (nonatomic) IBInspectable NSString * localizedText;

@end
