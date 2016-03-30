@protocol MWMEditorCellProtocol <NSObject>

@required
- (void)cell:(UITableViewCell *)cell changedText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cellSelect:(UITableViewCell *)cell;

@optional
- (void)fieldIsCorrect:(BOOL)isCorrect;

@end
