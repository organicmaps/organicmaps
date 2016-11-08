#import <UIKit/UIKit.h>
@class MWMDownloadTransitMapAlert;

@interface MWMDownloaderDialogHeader : UIView

@property (weak, nonatomic) IBOutlet UIButton * headerButton;
@property (weak, nonatomic) IBOutlet UIImageView * expandImage;

+ (instancetype)headerForOwnerAlert:(MWMDownloadTransitMapAlert *)alert;
- (void)layoutSizeLabel;

- (void)setTitle:(NSString *)title size:(NSString *)size;

@end
