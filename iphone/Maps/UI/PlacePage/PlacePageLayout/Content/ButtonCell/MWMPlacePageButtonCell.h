#import "MWMPlacePageData.h"
#import "MWMTableViewCell.h"
#import "MWMTypes.h"

@protocol MWMPlacePageButtonsProtocol;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)configWithTitle:(NSString *)title
                 action:(MWMVoidBlock)action
          isInsetButton:(BOOL)isInsetButton;

- (void)configForRow:(place_page::ButtonsRows)row
        withAction:(MWMVoidBlock)action;

- (void)setEnabled:(BOOL)enabled;
- (BOOL)isEnabled;

@end
