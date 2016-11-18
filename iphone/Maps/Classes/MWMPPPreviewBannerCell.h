@interface MWMPPPreviewBannerCell : UITableViewCell

- (void)configWithTitle:(NSString *)title
                content:(NSString *)content
                  adURL:(NSURL *)adURL
               imageURL:(NSURL *)imageURL;

- (void)configImageInOpenState;
- (void)configImageInPreviewState;

@end
