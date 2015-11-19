@class MWMPageController;

@interface MWMWhatsNewController : UIViewController

@property (nonatomic) NSUInteger pageIndex;
@property (weak, nonatomic) MWMPageController * pageController;

- (void)updateForSize:(CGSize)size;

@end
