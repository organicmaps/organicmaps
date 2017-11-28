@class MWMEditorTextTableViewCell;
@class MWMTableViewCell;

@protocol MWMEditorCellProtocol <NSObject>

- (void)cell:(MWMTableViewCell *)cell changedText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cellSelect:(UITableViewCell *)cell;
- (void)tryToChangeInvalidStateForCell:(MWMTableViewCell *)cell;

@end

@protocol MWMEditorAdditionalName <MWMEditorCellProtocol>

- (void)editAdditionalNameLanguage:(NSInteger)selectedLangCode;

@end
