@class MWMEditorTextTableViewCell;

@protocol MWMEditorCellProtocol <NSObject>

@required
- (void)cell:(MWMEditorTextTableViewCell *)cell changedText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cellSelect:(UITableViewCell *)cell;
- (void)tryToChangeInvalidStateForCell:(MWMEditorTextTableViewCell *)cell;

@optional
- (void)fieldIsCorrect:(BOOL)isCorrect;

@end
