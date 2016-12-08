#import "MWMPlacePageTaxiCell.h"
#import "MWMPlacePageProtocol.h"

@interface MWMPlacePageTaxiCell()

@property(weak, nonatomic) id<MWMPlacePageButtonsProtocol> delegate;

@end

@implementation MWMPlacePageTaxiCell

- (IBAction)orderTaxi { [self.delegate taxiTo]; }

@end
