#import "MWMTableViewCell.h"

namespace search { class Result; }

@interface MWMSearchCell : MWMTableViewCell

- (void)config:(search::Result const &)result
    localizedTypeName:(NSString *)localizedTypeName;
@end
