#import "MWMNoteButtonCell.h"

@interface MWMNoteButtonCell ()

@property (weak, nonatomic) IBOutlet UIButton * button;
@property (weak, nonatomic) id<MWMEditorCellProtocol> delegate;

@end

@implementation MWMNoteButtonCell

- (void)configureWithDelegate:(id<MWMEditorCellProtocol>)delegate title:(NSString *)title
{
  [self.button setTitle:title forState:UIControlStateNormal];
  self.delegate = delegate;
}

- (IBAction)buttonTap
{
  [self.delegate cellSelect:self];
}

@end
