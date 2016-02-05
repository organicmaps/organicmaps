#import "MWMTableViewCell.h"

@interface MWMMapDownloaderTableViewCell : MWMTableViewCell

@property (nonatomic, readonly) CGFloat estimatedHeight;

- (void)setTitleText:(NSString *)text;
- (void)setDownloadSizeText:(NSString *)text;
- (void)setLastCell:(BOOL)isLast;

@end
