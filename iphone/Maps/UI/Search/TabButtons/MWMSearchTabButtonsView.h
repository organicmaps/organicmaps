@class MWMSearchTabButtonsView;

@protocol MWMSearchTabButtonsViewProtocol <NSObject>

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender;

@end

@interface MWMSearchTabButtonsView : UIView

@property (nonatomic) BOOL selected;

@property (nonatomic) UIImage * iconImage;
@property (copy, nonatomic) NSString * localizedText;

@end
