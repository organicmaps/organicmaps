@interface MWMBottomMenuCollectionViewCell : UICollectionViewCell

@property(weak, nonatomic) IBOutlet UIImageView * icon;

- (void)configureWithIconName:(NSString *)iconName
                        label:(NSString *)label
                   badgeCount:(NSUInteger)badgeCount;

- (void)highlighted:(BOOL)highlighted;

@end
