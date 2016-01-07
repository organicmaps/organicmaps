#import "MWMCuisineEditorTableViewCell.h"

@interface MWMCuisineEditorTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * label;
@property (weak, nonatomic) IBOutlet UIImageView * selectedIcon;

@property (weak, nonatomic) id<MWMCuisineEditorTableViewCellProtocol> delegate;
@property (copy, nonatomic) NSString * key;

@end

@implementation MWMCuisineEditorTableViewCell

- (void)configWithDelegate:(id<MWMCuisineEditorTableViewCellProtocol>)delegate key:(NSString *)key selected:(BOOL)selected
{
  self.delegate = delegate;
  self.key = key;
  NSString * cuisine = [NSString stringWithFormat:@"cuisine_%@", key];
  self.label.text = [L(cuisine) capitalizedStringWithLocale:[NSLocale currentLocale]];
  self.selectedIcon.hidden = !selected;
}

#pragma mark - Actions

- (IBAction)selectAction
{
  BOOL const selected = self.selectedIcon.hidden;
  self.selectedIcon.hidden = !selected;
  [self.delegate change:self.key selected:selected];
}

@end
