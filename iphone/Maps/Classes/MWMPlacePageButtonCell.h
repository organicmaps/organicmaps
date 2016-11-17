#import "MWMPlacePageData.h"
#import "MWMTableViewCell.h"

@protocol MWMPlacePageButtonsProtocol;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)configForRow:(place_page::ButtonsRows)row
        withDelegate:(id<MWMPlacePageButtonsProtocol>)delegate;

- (void)setEnabled:(BOOL)enabled;
- (BOOL)isEnabled;

@end
