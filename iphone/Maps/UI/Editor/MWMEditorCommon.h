#include "indexer/yes_no_unknown.hpp"

@class MWMEditorTextTableViewCell;
@class MWMTableViewCell;

@protocol MWMEditorCellProtocol <NSObject>

- (void)cell:(MWMTableViewCell *)cell changedText:(NSString *)changeText;
- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch;
- (void)cell:(UITableViewCell *)cell changeSegmented:(YesNoUnknown)changeSegmented;
- (void)cellDidPressButton:(UITableViewCell *)cell;
- (void)tryToChangeInvalidStateForCell:(MWMTableViewCell *)cell;

@end

@protocol MWMEditorAdditionalName <MWMEditorCellProtocol>

- (void)editAdditionalNameLanguage:(NSInteger)selectedLangCode;

@end
