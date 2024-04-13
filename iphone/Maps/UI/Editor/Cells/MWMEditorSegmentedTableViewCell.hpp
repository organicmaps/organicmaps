#import "MWMEditorCommon.h"
#import "MWMTableViewCell.h"
#include "indexer/yes_no_unknown.hpp"

@interface MWMEditorSegmentedTableViewCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMEditorCellProtocol>)delegate
                      icon:(UIImage *)icon
                      text:(NSString *)text
                     value:(YesNoUnknown)value;

@end
