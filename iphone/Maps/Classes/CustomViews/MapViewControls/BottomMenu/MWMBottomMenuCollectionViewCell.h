@interface MWMBottomMenuCollectionViewCell : UICollectionViewCell

@property (weak, nonatomic) IBOutlet UIImageView * icon;

- (void)configureWithImage:(UIImage *)image
          highlightedImage:(UIImage *)highlightedImage
                    label:(NSString *)label
               badgeCount:(NSUInteger)badgeCount;

- (void)configureWithImageName:(NSString *)imageName
                         label:(NSString *)label
                    badgeCount:(NSUInteger)badgeCount;

- (void)highlighted:(BOOL)highlighted;

@end
