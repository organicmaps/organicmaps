@class MWMPlacePageData;

@protocol MWMPPPreviewLayoutHelperDelegate<NSObject>

- (void)heightWasChanged;

@end

@interface MWMPPPreviewLayoutHelper : NSObject

- (instancetype)initWithTableView:(UITableView *)tableView;
- (void)configWithData:(MWMPlacePageData *)data;
- (void)setDelegate:(id<MWMPPPreviewLayoutHelperDelegate>)delegate;
- (UITableViewCell *)cellForRowAtIndexPath:(NSIndexPath *)indexPath
                                  withData:(MWMPlacePageData *)data;
- (void)rotateDirectionArrowToAngle:(CGFloat)angle;
- (void)setDistanceToObject:(NSString *)distance;
- (void)insertRowAtTheEnd;
- (CGFloat)height;

- (void)layoutInOpenState:(BOOL)isOpen;

@end
