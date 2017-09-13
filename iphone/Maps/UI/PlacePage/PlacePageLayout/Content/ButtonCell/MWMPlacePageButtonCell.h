#import "MWMTableViewCell.h"

@protocol MWMPlacePageButtonsProtocol;

namespace place_page
{
enum class ButtonsRows;
}  // namespace place_page

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)configWithTitle:(NSString *)title
                 action:(MWMVoidBlock)action
          isInsetButton:(BOOL)isInsetButton;

- (void)configForRow:(place_page::ButtonsRows)row
        withAction:(MWMVoidBlock)action;

- (void)setEnabled:(BOOL)enabled;
- (BOOL)isEnabled;

- (place_page::ButtonsRows)rowType;

@end
