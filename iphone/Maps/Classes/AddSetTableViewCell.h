@protocol AddSetTableViewCellProtocol <NSObject>

- (void)onDone:(NSString *)text;

@end

@interface AddSetTableViewCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UITextField * textField;

@property (weak, nonatomic) id<AddSetTableViewCellProtocol> delegate;

@end
