@interface MWMTableViewCell : UITableViewCell

@property(nonatomic) BOOL isSeparatorHidden;

- (void)awakeFromNib NS_REQUIRES_SUPER;
- (void)configure;

@end

@interface MWMTableViewSubtitleCell : MWMTableViewCell

@end
