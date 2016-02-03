@interface MWMMapDownloaderTableViewCell : UITableViewCell

@property (nonatomic, readonly) CGFloat estimatedHeight;

@property (weak, nonatomic) IBOutlet UIView * stateWrapper;
@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * downloadSize;
@property (weak, nonatomic) IBOutlet UIView * separator;

@end
