@interface MWMBottomMenuCollectionViewCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UIImageView * icon;

- (void)configureWithImageName:(NSString *)imageName
                         label:(NSString *)label
                    badgeCount:(NSUInteger)badgeCount;

@end
