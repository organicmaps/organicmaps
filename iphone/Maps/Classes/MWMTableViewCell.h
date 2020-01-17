@interface MWMTableViewCell : UITableViewCell

@property(nonatomic) BOOL isSeparatorHidden;

- (void)awakeFromNib NS_REQUIRES_SUPER;

@end

@interface MWMTableViewSubtitleCell : MWMTableViewCell

@end
