#import "MWMPlacePageData.h"
#import "MWMPlacePageEntity.h"
#import "MWMTableViewCell.h"

@class MWMPlacePageViewManager;

@protocol MWMPlacePageButtonsProtocol;

@interface MWMPlacePageButtonCell : MWMTableViewCell

- (void)config:(MWMPlacePageViewManager *)manager
       forType:(MWMPlacePageCellType)type NS_DEPRECATED_IOS(7_0, 8_0, "Use configForRow:withDelegate: instead");

- (void)configForRow:(place_page::ButtonsRows)row
        withDelegate:(id<MWMPlacePageButtonsProtocol>)delegate NS_AVAILABLE_IOS(8_0);

- (void)setEnabled:(BOOL)enabled NS_AVAILABLE_IOS(8_0);
- (BOOL)isEnabled NS_AVAILABLE_IOS(8_0);

@end
