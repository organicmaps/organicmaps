@class MWMPlacePageData;

@interface MWMPPPreviewLayoutHelper : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView;
- (void)configWithData:(MWMPlacePageData *)data;
- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath
                                  withData:(MWMPlacePageData *)data;
- (void)rotateDirectionArrowToAngle:(CGFloat)angle;
- (void)setDistanceToObject:(NSString *)distance;
- (CGFloat)height;

- (void)layoutInOpenState:(BOOL)isOpen;

@end
