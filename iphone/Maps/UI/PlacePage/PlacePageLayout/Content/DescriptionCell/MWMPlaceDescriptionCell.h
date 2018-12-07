#import "MWMTableViewCell.h"

@protocol MWMPlacePageButtonsProtocol;

@interface MWMPlaceDescriptionCell : MWMTableViewCell

- (void)configureWithDescription:(NSString *)text delegate:(id<MWMPlacePageButtonsProtocol>)delegate;

@end
