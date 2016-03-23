#import "MWMStreetEditorCommonTableViewCell.h"

@interface MWMStreetEditorCommonTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UIImageView * selectedIcon;

@property (weak, nonatomic) id<MWMStreetEditorCommonTableViewCellProtocol> delegate;

@end

@implementation MWMStreetEditorCommonTableViewCell

- (void)configWithDelegate:(id<MWMStreetEditorCommonTableViewCellProtocol>)delegate street:(NSString *)street selected:(BOOL)selected
{
  self.delegate = delegate;
  self.label.text = street;
  self.selectedIcon.hidden = !selected;
}

#pragma mark - Actions

- (IBAction)selectAction
{
  self.selectedIcon.hidden = !self.selectedIcon.hidden;
  [self.delegate selectCell:self];
}

@end
