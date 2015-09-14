@interface MWMSearchBookmarksCell : UITableViewCell

@property (nonatomic) BOOL isLightTheme;

- (void)configForIndex:(NSInteger)index;

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

@end
