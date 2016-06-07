#import "MWMTypes.h"

@interface MWMEditorAdditionalNamesHeader : UIView

+ (instancetype)header:(TMWMVoidBlock)toggleBlock;

- (void)setShowAdditionalNames:(BOOL)showAdditionalNames;

@end
