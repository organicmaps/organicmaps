@class MWMPlacePageData;

@interface MWMPPPreviewLayoutHelper : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView;
- (void)registerCells;
- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath
                                  withData:(MWMPlacePageData *)data;

- (void)rotateDirectionArrowToAngle:(CGFloat)angle;
- (void)setDistanceToObject:(NSString *)distance;
- (CGFloat)height;

@end
