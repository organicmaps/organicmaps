@interface MWMEditorAdditionalNamesHeader : UIView

+ (instancetype)header:(MWMVoidBlock)toggleBlock;

- (void)setShowAdditionalNames:(BOOL)showAdditionalNames;
- (void)setAdditionalNamesVisible:(BOOL)visible;

@end
