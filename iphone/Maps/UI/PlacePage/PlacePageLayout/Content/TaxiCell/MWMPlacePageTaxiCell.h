#import "MWMTableViewCell.h"

@protocol MWMPlacePageButtonsProtocol;

@interface MWMPlacePageTaxiCell : MWMTableViewCell

- (void)setDelegate:(id<MWMPlacePageButtonsProtocol>)delegate;

@end
