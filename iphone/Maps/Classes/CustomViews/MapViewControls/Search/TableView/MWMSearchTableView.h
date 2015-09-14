@interface MWMSearchTableView : UIView

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (weak, nonatomic) IBOutlet UIView * noResultsView;
@property (weak, nonatomic) IBOutlet UIImageView * noResultsImage;
@property (weak, nonatomic) IBOutlet UILabel * noResultsText;

@end
