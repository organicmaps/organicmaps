@interface MWMSearchHistoryRequestCell : UITableViewCell

@property (nonatomic) BOOL isLightTheme;

- (void)config:(NSString *)title;

+ (CGFloat)defaultCellHeight;
- (CGFloat)cellHeight;

@end
