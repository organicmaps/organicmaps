#import "MWMEditorAddAdditionalNameTableViewCell.h"

@interface MWMEditorAddAdditionalNameTableViewCell ()

@property(weak, nonatomic) id<MWMEditorAdditionalName> delegate;

@end

@implementation MWMEditorAddAdditionalNameTableViewCell

- (void)configWithDelegate:(id<MWMEditorAdditionalName>)delegate { self.delegate = delegate; }
- (IBAction)addLanguageTap { [self.delegate editAdditionalNameLanguage:NSNotFound]; }
@end
