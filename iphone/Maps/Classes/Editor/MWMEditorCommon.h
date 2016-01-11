@protocol MWMEditorCellProtocol <NSObject>

- (void)cell:(UITableViewCell *)cell changeText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cellSelect:(UITableViewCell *)cell;

@end
