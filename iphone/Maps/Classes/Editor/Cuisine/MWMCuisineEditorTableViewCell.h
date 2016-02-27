#include "std/string.hpp"

@protocol MWMCuisineEditorTableViewCellProtocol <NSObject>

- (void)change:(string const &)key selected:(BOOL)selected;

@end

@interface MWMCuisineEditorTableViewCell : UITableViewCell

- (void)configWithDelegate:(id<MWMCuisineEditorTableViewCellProtocol>)delegate
                       key:(string const &)key
               translation:(string const &)translation
                  selected:(BOOL)selected;

@end
