@protocol MWMEditorCellProtocol <NSObject>

- (void)cellBeginEditing:(UITableViewCell *)cell;
- (void)cell:(UITableViewCell *)cell changeText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cellSelect:(UITableViewCell *)cell;

@end
