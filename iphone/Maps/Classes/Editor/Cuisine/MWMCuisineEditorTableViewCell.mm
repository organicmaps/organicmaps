#import "MWMCuisineEditorTableViewCell.h"

@interface MWMCuisineEditorTableViewCell ()
{
  string m_key;
}

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UIImageView * selectedIcon;

@property (weak, nonatomic) id<MWMCuisineEditorTableViewCellProtocol> delegate;

@end

@implementation MWMCuisineEditorTableViewCell

- (void)configWithDelegate:(id<MWMCuisineEditorTableViewCellProtocol>)delegate
                       key:(string const &)key
               translation:(string const &)translation
                  selected:(BOOL)selected
{
  self.delegate = delegate;
  m_key = key;
  self.label.text = @(translation.c_str());
  self.selectedIcon.hidden = !selected;
}

#pragma mark - Actions

- (IBAction)selectAction
{
  BOOL const selected = self.selectedIcon.hidden;
  self.selectedIcon.hidden = !selected;
  [self.delegate change:m_key selected:selected];
}

@end
